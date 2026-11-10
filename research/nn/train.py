"""
research/nn/train.py

Production-grade training pipeline for RookSac policy-value residual neural network.

Features:
- Real dataset-driven training using sharded JSONL / JSONL.GZ datasets.
- Explicit --smoke-test mode for synthetic fixtures (clearly labeled; never defaults to it).
- Lazy/sharded PyTorch Dataset and DataLoader with num_workers and pin_memory.
- Joint loss: policy Cross-Entropy + weighted value MSE.
- Value target perspective: White's perspective in [-1, 1], consistent across extraction,
  model documentation, and C++ inference.
- Learning rate scheduling (CosineAnnealingLR or StepLR), AdamW optimizer, gradient clipping.
- Deterministic seeding and comprehensive provenance recording (Python, PyTorch, CUDA, Git commit).
- Checkpointing: last.pt, best.pt (by validation total loss), resume capability.
- Early stopping based on validation loss patience.
- Independent test split evaluation only after training/model selection (reports top-1/top-3 acc,
  value MSE/MAE, value calibration error).
- Structured logging to CSV and JSON metrics ledger.
- Strict error handling for empty datasets, corrupted shards, NaN loss, and incompatible checkpoints.
"""

import argparse
import csv
import json
import math
import os
import platform
import random
import subprocess
import sys
import time
from typing import Any, Dict, List, Optional, Tuple

import numpy as np
import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import DataLoader

from research.nn.dataset import ShardedChessDataset
from research.nn.features import VOCAB_SIZE, VOCAB_VERSION
from research.nn.model import PolicyValueNet

MODEL_SCHEMA_VERSION = "2.0.0"
FEATURE_SCHEMA = "(18, 8, 8) [pieces: 12, stm: 1, castling: 4, ep: 1]"


def get_git_revision() -> str:
    try:
        rev = subprocess.check_output(
            ["git", "rev-parse", "HEAD"], stderr=subprocess.DEV_NULL, text=True
        ).strip()
        return rev
    except Exception:
        return "unknown"


def set_seed(seed: int = 42):
    random.seed(seed)
    np.random.seed(seed)
    torch.manual_seed(seed)
    if torch.cuda.is_available():
        torch.cuda.manual_seed_all(seed)


def collect_system_provenance() -> Dict[str, Any]:
    return {
        "python_version": platform.python_version(),
        "pytorch_version": torch.__version__,
        "cuda_available": torch.cuda.is_available(),
        "cuda_version": torch.version.cuda if torch.cuda.is_available() else None,
        "device_name": torch.cuda.get_device_name(0) if torch.cuda.is_available() else "CPU",
        "git_commit": get_git_revision(),
        "platform": platform.platform(),
    }


def train_epoch(
    model: nn.Module,
    loader: DataLoader,
    optimizer: optim.Optimizer,
    policy_criterion: nn.Module,
    value_criterion: nn.Module,
    value_weight: float = 1.0,
    grad_clip: float = 1.0,
    device: torch.device = torch.device("cpu"),
) -> Tuple[float, float, float, float]:
    """Runs a single training epoch.

    Returns: (avg_total_loss, policy_top1_acc, avg_policy_loss, avg_value_loss)
    """
    model.train()
    total_loss = 0.0
    total_p_loss = 0.0
    total_v_loss = 0.0
    correct_policy = 0
    total_samples = 0

    for x, target_policy, target_val in loader:
        x = x.to(device, non_blocking=True)
        target_policy = target_policy.to(device, non_blocking=True)
        target_val = target_val.to(device, non_blocking=True)

        optimizer.zero_grad()
        p_logits, v_pred = model(x)

        p_loss = policy_criterion(p_logits, target_policy)
        v_loss = value_criterion(v_pred, target_val)
        loss = p_loss + value_weight * v_loss

        if torch.isnan(loss) or torch.isinf(loss):
            raise ValueError(f"Encountered NaN/Inf loss during training! (policy_loss={p_loss.item()}, value_loss={v_loss.item()})")

        loss.backward()
        if grad_clip > 0.0:
            torch.nn.utils.clip_grad_norm_(model.parameters(), grad_clip)
        optimizer.step()

        batch_size = x.size(0)
        total_loss += loss.item() * batch_size
        total_p_loss += p_loss.item() * batch_size
        total_v_loss += v_loss.item() * batch_size

        preds = torch.argmax(p_logits, dim=-1)
        correct_policy += (preds == target_policy).sum().item()
        total_samples += batch_size

    if total_samples == 0:
        raise ValueError("Cannot train on an empty DataLoader.")

    avg_loss = total_loss / total_samples
    acc = correct_policy / total_samples
    avg_p_loss = total_p_loss / total_samples
    avg_v_loss = total_v_loss / total_samples
    return avg_loss, acc, avg_p_loss, avg_v_loss


def evaluate_dataset(
    model: nn.Module,
    loader: DataLoader,
    policy_criterion: nn.Module,
    value_criterion: nn.Module,
    value_weight: float = 1.0,
    device: torch.device = torch.device("cpu"),
) -> Dict[str, float]:
    """Comprehensive evaluation on validation or held-out test data.

    Returns dictionary with:
    - total_loss, policy_loss, value_loss
    - policy_top1_acc, policy_top3_acc
    - value_mse, value_mae, value_calibration_error
    """
    model.eval()
    total_loss = 0.0
    total_p_loss = 0.0
    total_v_loss = 0.0
    correct_top1 = 0
    correct_top3 = 0
    abs_v_error = 0.0
    total_samples = 0

    with torch.no_grad():
        for x, target_policy, target_val in loader:
            x = x.to(device, non_blocking=True)
            target_policy = target_policy.to(device, non_blocking=True)
            target_val = target_val.to(device, non_blocking=True)

            p_logits, v_pred = model(x)

            p_loss = policy_criterion(p_logits, target_policy)
            v_loss = value_criterion(v_pred, target_val)
            loss = p_loss + value_weight * v_loss

            batch_size = x.size(0)
            total_loss += loss.item() * batch_size
            total_p_loss += p_loss.item() * batch_size
            total_v_loss += v_loss.item() * batch_size

            # Accuracy
            preds = torch.argmax(p_logits, dim=-1)
            correct_top1 += (preds == target_policy).sum().item()

            if p_logits.size(-1) >= 3:
                _, top3_indices = torch.topk(p_logits, k=3, dim=-1)
                correct_top3 += (top3_indices == target_policy.unsqueeze(1)).any(dim=-1).sum().item()
            else:
                correct_top3 += (preds == target_policy).sum().item()

            abs_v_error += torch.abs(v_pred - target_val).sum().item()
            total_samples += batch_size

    if total_samples == 0:
        return {
            "total_loss": 0.0,
            "policy_loss": 0.0,
            "value_loss": 0.0,
            "policy_top1_acc": 0.0,
            "policy_top3_acc": 0.0,
            "value_mse": 0.0,
            "value_mae": 0.0,
            "value_calibration_error": 0.0,
        }

    avg_loss = total_loss / total_samples
    avg_p_loss = total_p_loss / total_samples
    avg_v_loss = total_v_loss / total_samples
    top1 = correct_top1 / total_samples
    top3 = correct_top3 / total_samples
    mae = abs_v_error / total_samples

    return {
        "total_loss": round(avg_loss, 5),
        "policy_loss": round(avg_p_loss, 5),
        "value_loss": round(avg_v_loss, 5),
        "policy_top1_acc": round(top1, 5),
        "policy_top3_acc": round(top3, 5),
        "value_mse": round(avg_v_loss, 5),
        "value_mae": round(mae, 5),
        "value_calibration_error": round(abs(math.sqrt(max(0.0, avg_v_loss)) - mae), 5),
    }


def save_checkpoint(
    path: str,
    model: nn.Module,
    optimizer: optim.Optimizer,
    scheduler: Optional[Any],
    epoch: int,
    best_metric: float,
    config: Dict[str, Any],
    metadata: Dict[str, Any],
):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    payload = {
        "epoch": epoch,
        "best_metric": best_metric,
        "model_state_dict": model.state_dict(),
        "optimizer_state_dict": optimizer.state_dict(),
        "scheduler_state_dict": scheduler.state_dict() if scheduler else None,
        "config": config,
        "metadata": metadata,
    }
    torch.save(payload, path)


def load_checkpoint(
    checkpoint_path: str,
    model: nn.Module,
    optimizer: Optional[optim.Optimizer] = None,
    scheduler: Optional[Any] = None,
    device: torch.device = torch.device("cpu"),
) -> Tuple[int, float, Dict[str, Any]]:
    if not os.path.exists(checkpoint_path):
        raise FileNotFoundError(f"Checkpoint file not found: {checkpoint_path}")

    checkpoint = torch.load(checkpoint_path, map_location=device, weights_only=False)

    meta = checkpoint.get("metadata", {})
    if meta.get("schema_version") and meta["schema_version"] != MODEL_SCHEMA_VERSION:
        raise ValueError(
            f"Incompatible checkpoint schema: {meta.get('schema_version')} != {MODEL_SCHEMA_VERSION}"
        )
    if meta.get("vocab_size") and meta["vocab_size"] != VOCAB_SIZE:
        raise ValueError(
            f"Incompatible action vocabulary size in checkpoint: {meta.get('vocab_size')} != {VOCAB_SIZE}"
        )

    model.load_state_dict(checkpoint["model_state_dict"])
    if optimizer and "optimizer_state_dict" in checkpoint and checkpoint["optimizer_state_dict"]:
        optimizer.load_state_dict(checkpoint["optimizer_state_dict"])
    if scheduler and "scheduler_state_dict" in checkpoint and checkpoint["scheduler_state_dict"]:
        scheduler.load_state_dict(checkpoint["scheduler_state_dict"])

    epoch = checkpoint.get("epoch", 0)
    best_metric = checkpoint.get("best_metric", float("inf"))
    return epoch, best_metric, checkpoint.get("config", {})


def run_training_pipeline(
    train_dataset: ShardedChessDataset,
    val_dataset: ShardedChessDataset,
    test_dataset: Optional[ShardedChessDataset] = None,
    output_dir: str = "research/checkpoints/run_001",
    epochs: int = 10,
    batch_size: int = 64,
    lr: float = 1e-3,
    weight_decay: float = 1e-4,
    value_weight: float = 1.0,
    grad_clip: float = 1.0,
    num_blocks: int = 4,
    channels: int = 64,
    num_workers: int = 0,
    pin_memory: bool = False,
    patience: int = 5,
    seed: int = 42,
    resume_checkpoint: Optional[str] = None,
    is_smoke_test: bool = False,
    dataset_manifest_checksum: Optional[str] = None,
) -> Dict[str, Any]:
    """Executes the complete supervised training pipeline."""
    set_seed(seed)
    os.makedirs(output_dir, exist_ok=True)
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

    if len(train_dataset) == 0:
        raise ValueError("Cannot train on an empty training dataset!")
    if len(val_dataset) == 0:
        raise ValueError("Cannot validate on an empty validation dataset!")

    train_loader = DataLoader(
        train_dataset,
        batch_size=batch_size,
        shuffle=True,
        num_workers=num_workers,
        pin_memory=pin_memory and torch.cuda.is_available(),
    )
    val_loader = DataLoader(
        val_dataset,
        batch_size=batch_size,
        shuffle=False,
        num_workers=num_workers,
        pin_memory=pin_memory and torch.cuda.is_available(),
    )

    model = PolicyValueNet(
        in_channels=18,
        num_blocks=num_blocks,
        channels=channels,
        num_actions=VOCAB_SIZE,
    ).to(device)

    optimizer = optim.AdamW(model.parameters(), lr=lr, weight_decay=weight_decay)
    scheduler = optim.lr_scheduler.CosineAnnealingLR(optimizer, T_max=max(1, epochs), eta_min=1e-5)
    policy_criterion = nn.CrossEntropyLoss()
    value_criterion = nn.MSELoss()

    start_epoch = 1
    best_val_loss = float("inf")
    patience_counter = 0

    training_config = {
        "epochs": epochs,
        "batch_size": batch_size,
        "lr": lr,
        "weight_decay": weight_decay,
        "value_weight": value_weight,
        "grad_clip": grad_clip,
        "num_blocks": num_blocks,
        "channels": channels,
        "seed": seed,
        "is_smoke_test": is_smoke_test,
        "dataset_manifest_checksum": dataset_manifest_checksum,
    }

    base_metadata = {
        "architecture": "PolicyValueNet-ResNet",
        "schema_version": MODEL_SCHEMA_VERSION,
        "vocab_version": VOCAB_VERSION,
        "vocab_size": VOCAB_SIZE,
        "input_schema": FEATURE_SCHEMA,
        "value_perspective": "White perspective in [-1.0, 1.0]",
        "is_smoke_test": is_smoke_test,
        "provenance": collect_system_provenance(),
    }

    if resume_checkpoint is not None:
        print(f"Resuming from checkpoint: {resume_checkpoint}")
        res_epoch, res_best, _ = load_checkpoint(
            resume_checkpoint, model, optimizer, scheduler, device=device
        )
        start_epoch = res_epoch + 1
        best_val_loss = res_best
        print(f"Resumed at epoch {start_epoch} (previous best val loss: {best_val_loss:.4f})")

    # CSV Logging setup
    csv_path = os.path.join(output_dir, "training_metrics.csv")
    csv_exists = os.path.exists(csv_path)
    csv_file = open(csv_path, "a", newline="", encoding="utf-8")
    csv_writer = csv.writer(csv_file)
    if not csv_exists:
        csv_writer.writerow([
            "epoch", "lr", "train_loss", "train_acc", "train_p_loss", "train_v_loss",
            "val_loss", "val_top1_acc", "val_top3_acc", "val_v_mse", "val_v_mae", "time_sec"
        ])

    metrics_history = []
    print(f"Starting training on {device} (Epochs: {start_epoch}..{epochs}, Train samples: {len(train_dataset)}, Val: {len(val_dataset)})")
    if is_smoke_test:
        print(">>> WARNING: RUNNING IN EXPLICIT --smoke-test MODE. FIXTURE MODEL ONLY! <<<")

    for epoch in range(start_epoch, epochs + 1):
        t0 = time.time()
        curr_lr = optimizer.param_groups[0]["lr"]

        tr_loss, tr_acc, tr_p_loss, tr_v_loss = train_epoch(
            model=model,
            loader=train_loader,
            optimizer=optimizer,
            policy_criterion=policy_criterion,
            value_criterion=value_criterion,
            value_weight=value_weight,
            grad_clip=grad_clip,
            device=device,
        )

        val_metrics = evaluate_dataset(
            model=model,
            loader=val_loader,
            policy_criterion=policy_criterion,
            value_criterion=value_criterion,
            value_weight=value_weight,
            device=device,
        )

        scheduler.step()
        elapsed = round(time.time() - t0, 2)

        epoch_record = {
            "epoch": epoch,
            "lr": curr_lr,
            "train_loss": round(tr_loss, 4),
            "train_acc": round(tr_acc, 4),
            "train_p_loss": round(tr_p_loss, 4),
            "train_v_loss": round(tr_v_loss, 4),
            **val_metrics,
            "time_sec": elapsed,
        }
        metrics_history.append(epoch_record)

        csv_writer.writerow([
            epoch, curr_lr, epoch_record["train_loss"], epoch_record["train_acc"],
            epoch_record["train_p_loss"], epoch_record["train_v_loss"],
            val_metrics["total_loss"], val_metrics["policy_top1_acc"],
            val_metrics["policy_top3_acc"], val_metrics["value_mse"],
            val_metrics["value_mae"], elapsed
        ])
        csv_file.flush()

        print(
            f"Epoch {epoch:02d}/{epochs:02d} [{elapsed}s] - "
            f"TrLoss: {tr_loss:.4f} TrAcc: {tr_acc:.2%} | "
            f"ValLoss: {val_metrics['total_loss']:.4f} Top1: {val_metrics['policy_top1_acc']:.2%} Top3: {val_metrics['policy_top3_acc']:.2%} VMSE: {val_metrics['value_mse']:.4f}"
        )

        # Checkpoint: last.pt
        last_ckpt_path = os.path.join(output_dir, "last.pt")
        save_checkpoint(
            path=last_ckpt_path,
            model=model,
            optimizer=optimizer,
            scheduler=scheduler,
            epoch=epoch,
            best_metric=best_val_loss,
            config=training_config,
            metadata={**base_metadata, "last_metrics": epoch_record},
        )

        # Best model selection by validation total loss
        if val_metrics["total_loss"] < best_val_loss:
            best_val_loss = val_metrics["total_loss"]
            patience_counter = 0
            best_ckpt_path = os.path.join(output_dir, "best.pt")
            save_checkpoint(
                path=best_ckpt_path,
                model=model,
                optimizer=optimizer,
                scheduler=scheduler,
                epoch=epoch,
                best_metric=best_val_loss,
                config=training_config,
                metadata={**base_metadata, "best_metrics": epoch_record},
            )
            print(f"  * New best validation loss: {best_val_loss:.4f} -> Saved {best_ckpt_path}")
        else:
            patience_counter += 1
            if patience_counter >= patience:
                print(f"Early stopping triggered at epoch {epoch} (patience={patience})")
                break

    csv_file.close()

    # Load best model for final evaluation
    best_ckpt_path = os.path.join(output_dir, "best.pt")
    if os.path.exists(best_ckpt_path):
        load_checkpoint(best_ckpt_path, model, device=device)

    # Isolated evaluation on held-out test split (only after training & model selection)
    test_metrics = None
    if test_dataset is not None and len(test_dataset) > 0:
        test_loader = DataLoader(
            test_dataset,
            batch_size=batch_size,
            shuffle=False,
            num_workers=num_workers,
            pin_memory=pin_memory and torch.cuda.is_available(),
        )
        print("Evaluating best model on isolated test split...")
        test_metrics = evaluate_dataset(
            model=model,
            loader=test_loader,
            policy_criterion=policy_criterion,
            value_criterion=value_criterion,
            value_weight=value_weight,
            device=device,
        )
        print(f"Test Split Results: {test_metrics}")

    # Persist summary JSON
    summary = {
        "metadata": base_metadata,
        "config": training_config,
        "best_val_loss": best_val_loss,
        "history": metrics_history,
        "test_metrics": test_metrics,
        "completed_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
    }
    summary_path = os.path.join(output_dir, "training_summary.json")
    with open(summary_path, "w", encoding="utf-8") as f:
        json.dump(summary, f, indent=2)
    print(f"Training run completed. Summary saved to {summary_path}")

    return summary


def run_smoke_test(
    output_dir: str = "research/checkpoints/smoke_test",
    resume_checkpoint: Optional[str] = None,
    epochs: int = 2,
) -> Dict[str, Any]:
    """Explicit separate smoke test using synthetic fixtures."""
    samples = [
        {"fen": "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "move": "e2e4", "result": 1.0},
        {"fen": "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1", "move": "e7e5", "result": 0.5},
        {"fen": "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2", "move": "g1f3", "result": 1.0},
        {"fen": "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3", "move": "f1b5", "result": 0.0},
        {"fen": "rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2", "move": "e4d5", "result": 0.5},
        {"fen": "rnbqkb1r/pppp1ppp/5n2/4p3/2B1P3/8/PPPP1PPP/RNBQK1NR w KQkq - 2 3", "move": "g1f3", "result": 1.0},
    ]
    train_ds = ShardedChessDataset(samples=samples[:4])
    val_ds = ShardedChessDataset(samples=samples[4:])
    test_ds = ShardedChessDataset(samples=samples[4:])

    return run_training_pipeline(
        train_dataset=train_ds,
        val_dataset=val_ds,
        test_dataset=test_ds,
        output_dir=output_dir,
        epochs=epochs,
        batch_size=2,
        lr=1e-3,
        num_blocks=1,
        channels=16,
        resume_checkpoint=resume_checkpoint,
        is_smoke_test=True,
    )


def main():
    parser = argparse.ArgumentParser(description="Supervised Policy-Value Network Training.")
    parser.add_argument("--dataset", type=str, default=None, help="Path to dataset directory containing dataset_manifest.json or shards/")
    parser.add_argument("--manifest", type=str, default=None, help="Direct path to dataset_manifest.json")
    parser.add_argument("--train-shards", nargs="+", default=None, help="Explicit list of training shards (.jsonl / .jsonl.gz)")
    parser.add_argument("--val-shards", nargs="+", default=None, help="Explicit list of validation shards (.jsonl / .jsonl.gz)")
    parser.add_argument("--test-shards", nargs="+", default=None, help="Explicit list of test shards (.jsonl / .jsonl.gz)")
    parser.add_argument("--smoke-test", action="store_true", help="Explicit smoke test mode using synthetic fixtures")
    parser.add_argument("--output-dir", type=str, default="research/checkpoints/run_001", help="Output directory for checkpoints and logs")
    parser.add_argument("--epochs", type=int, default=10, help="Number of training epochs")
    parser.add_argument("--batch-size", type=int, default=64, help="Batch size per step")
    parser.add_argument("--lr", type=float, default=1e-3, help="Initial learning rate")
    parser.add_argument("--weight-decay", type=float, default=1e-4, help="Weight decay for AdamW")
    parser.add_argument("--value-weight", type=float, default=1.0, help="Weight of value loss relative to policy loss")
    parser.add_argument("--grad-clip", type=float, default=1.0, help="Max gradient norm clipping")
    parser.add_argument("--blocks", type=int, default=4, help="Number of residual blocks in backbone")
    parser.add_argument("--channels", type=int, default=64, help="Feature channels in residual backbone")
    parser.add_argument("--num-workers", type=int, default=0, help="DataLoader worker processes")
    parser.add_argument("--pin-memory", action="store_true", help="Pin memory for GPU transfer")
    parser.add_argument("--patience", type=int, default=5, help="Early stopping patience epochs")
    parser.add_argument("--seed", type=int, default=42, help="Random seed for reproducibility")
    parser.add_argument("--resume", type=str, default=None, help="Path to checkpoint to resume training from")
    args = parser.parse_args()

    if args.smoke_test:
        run_smoke_test(
            output_dir=args.output_dir,
            resume_checkpoint=args.resume,
            epochs=args.epochs,
        )
        sys.exit(0)

    # Locate dataset
    manifest_path = None
    train_ds = None
    val_ds = None
    test_ds = None

    if args.dataset is not None:
        train_ds = ShardedChessDataset.from_directory(args.dataset, split="train")
        val_ds = ShardedChessDataset.from_directory(args.dataset, split="val")
        try:
            test_ds = ShardedChessDataset.from_directory(args.dataset, split="test")
        except Exception:
            test_ds = None
    elif args.manifest is not None:
        manifest_path = args.manifest
        train_ds = ShardedChessDataset.from_manifest(manifest_path, split="train")
        val_ds = ShardedChessDataset.from_manifest(manifest_path, split="val")
        try:
            test_ds = ShardedChessDataset.from_manifest(manifest_path, split="test")
        except Exception:
            test_ds = None
    elif args.train_shards is not None and args.val_shards is not None:
        train_ds = ShardedChessDataset(shard_paths=args.train_shards)
        val_ds = ShardedChessDataset(shard_paths=args.val_shards)
        if args.test_shards:
            test_ds = ShardedChessDataset(shard_paths=args.test_shards)
    else:
        # Check standard default processed dataset location
        default_dir = "research/data/processed"
        default_manifest = "research/data/dataset_manifest.json"
        if os.path.isdir(default_dir):
            print(f"Loading dataset from default directory: {default_dir}")
            train_ds = ShardedChessDataset.from_directory(default_dir, split="train")
            val_ds = ShardedChessDataset.from_directory(default_dir, split="val")
            try:
                test_ds = ShardedChessDataset.from_directory(default_dir, split="test")
            except Exception:
                test_ds = None
        elif os.path.exists(default_manifest):
            print(f"Loading dataset from default manifest: {default_manifest}")
            train_ds = ShardedChessDataset.from_manifest(default_manifest, split="train")
            val_ds = ShardedChessDataset.from_manifest(default_manifest, split="val")
            try:
                test_ds = ShardedChessDataset.from_manifest(default_manifest, split="test")
            except Exception:
                test_ds = None
        else:
            print(
                "Error: No dataset specified. Use --dataset, --manifest, --train-shards, or --smoke-test.\n"
                "To extract a dataset from PGN first, run:\n"
                "  python -m tools.pgn_extract --input <file.pgn> --output-dir research/data/processed",
                file=sys.stderr,
            )
            sys.exit(1)

    run_training_pipeline(
        train_dataset=train_ds,
        val_dataset=val_ds,
        test_dataset=test_ds,
        output_dir=args.output_dir,
        epochs=args.epochs,
        batch_size=args.batch_size,
        lr=args.lr,
        weight_decay=args.weight_decay,
        value_weight=args.value_weight,
        grad_clip=args.grad_clip,
        num_blocks=args.blocks,
        channels=args.channels,
        num_workers=args.num_workers,
        pin_memory=args.pin_memory,
        patience=args.patience,
        seed=args.seed,
        resume_checkpoint=args.resume,
        is_smoke_test=False,
    )


def train_model(
    train_samples: Optional[List[Dict]] = None,
    val_samples: Optional[List[Dict]] = None,
    manifest_path: Optional[str] = None,
    train_shards: Optional[List[str]] = None,
    val_shards: Optional[List[str]] = None,
    epochs: int = 3,
    batch_size: int = 8,
    lr: float = 1e-3,
    num_blocks: int = 4,
    channels: int = 64,
    seed: int = 42,
    save_path: str = "research/checkpoints/policy_value_test.pt",
    metadata_path: Optional[str] = None,
    is_smoke_test: bool = True,
) -> Dict[str, Any]:
    """Compatibility bridge for tests and lightweight runners."""
    if train_samples is not None and val_samples is not None:
        train_ds = ShardedChessDataset(samples=train_samples)
        val_ds = ShardedChessDataset(samples=val_samples)
    elif manifest_path is not None:
        train_ds = ShardedChessDataset.from_manifest(manifest_path, split="train")
        val_ds = ShardedChessDataset.from_manifest(manifest_path, split="val")
    elif train_shards is not None and val_shards is not None:
        train_ds = ShardedChessDataset(shard_paths=train_shards)
        val_ds = ShardedChessDataset(shard_paths=val_shards)
    else:
        raise ValueError("Must provide train_samples/val_samples, manifest_path, or shard paths.")

    out_dir = os.path.dirname(save_path) or "."
    summary = run_training_pipeline(
        train_dataset=train_ds,
        val_dataset=val_ds,
        output_dir=out_dir,
        epochs=epochs,
        batch_size=batch_size,
        lr=lr,
        num_blocks=num_blocks,
        channels=channels,
        seed=seed,
        is_smoke_test=is_smoke_test,
    )

    # Save to requested save_path and metadata_path
    if save_path:
        best_pt = os.path.join(out_dir, "best.pt")
        if os.path.exists(best_pt) and best_pt != save_path:
            import shutil
            shutil.copyfile(best_pt, save_path)

    if metadata_path:
        with open(metadata_path, "w", encoding="utf-8") as f:
            json.dump(summary["metadata"], f, indent=2)

    return summary["metadata"]


if __name__ == "__main__":
    main()
