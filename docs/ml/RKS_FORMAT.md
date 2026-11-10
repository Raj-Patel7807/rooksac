# RookSac Model File (.rks) Binary Specification

**Format Version**: 2  
**Magic Bytes**: `ROOKSAC2` (8 ASCII bytes: `0x52 0x4F 0x4F 0x4B 0x53 0x41 0x43 0x32`)  
**Status**: Authoritative

---

## 1. Overview & Objectives

The `.rks` binary container stores trained weights for the RookSac neural network (`PolicyValueNet`) for execution by the standalone C++20 forward-pass inference engine (`rooksac::neural::NeuralEvaluator`).

It provides:
- Single-file deployment with zero dynamic library dependencies (no ONNX runtime or PyTorch runtime needed).
- Pre-fused convolutional and batch-normalization parameters for optimal inference efficiency without runtime branching.
- Strict header metadata and safety bounds checks preventing buffer overflows, invalid allocations, and memory corruption.
- Full SHA-256 integrity checksum verification across the entire weights payload and metadata.
- Atomic export guarantee (temporary file write + validation + atomic rename).

---

## 2. Binary Layout

The `.rks` file consists of four contiguous sections:

```
+-------------------------------------------------------------+
| Header (64 bytes)                                           |
+-------------------------------------------------------------+
| JSON Metadata Payload (meta_bytes, UTF-8 encoded)           |
+-------------------------------------------------------------+
| Float32 Weights Payload (payload_bytes, IEEE 754 LE)        |
+-------------------------------------------------------------+
| SHA-256 Checksum (32 bytes binary digest)                   |
+-------------------------------------------------------------+
```

---

## 3. Header Specification (64 Bytes)

All integers are stored in **little-endian** byte order.

| Offset | Field Name | Type | Value / Description |
| :--- | :--- | :--- | :--- |
| `0x00` | `magic` | `char[8]` | `"ROOKSAC2"` |
| `0x08` | `format_version` | `uint32_t` | `2` |
| `0x0C` | `input_channels` | `uint32_t` | `18` (matches `(18, 8, 8)` position plane schema) |
| `0x10` | `num_blocks` | `uint32_t` | Count of residual blocks $B \in [1, 32]$ |
| `0x14` | `channels` | `uint32_t` | Backbone feature channels $C \in [8, 512]$ |
| `0x18` | `num_actions` | `uint32_t` | Action vocabulary count ($4240$ for Vocab v2.0.0) |
| `0x1C` | `meta_bytes` | `uint32_t` | Length of UTF-8 JSON metadata block |
| `0x20` | `payload_bytes` | `uint64_t` | Byte length of float32 weights payload ($N_{\text{floats}} \times 4$) |
| `0x28` | `reserved` | `uint8_t[24]`| Reserved for future extensions (must be zeroed) |

Total header size: **64 bytes**.

---

## 4. Metadata Payload Section

Immediately following the 64-byte header is a UTF-8 JSON payload of length `meta_bytes`.  
Contains provenance information:
```json
{
  "checkpoint_hash": "...",
  "source_git_commit": "...",
  "dataset_manifest_checksum": "...",
  "architecture": "PolicyValueNet-ResNet",
  "input_schema": "(18, 8, 8) [pieces: 12, stm: 1, castling: 4, ep: 1]",
  "action_schema": "v2.0.0 (4240 actions)",
  "training_metrics": { ... },
  "export_timestamp": "2026-11-10T08:25:00Z"
}
```

---

## 5. Weights Payload Order & Layout (IEEE 754 Float32 Little-Endian)

Tensors are serialized contiguously in Row-Major (C-order). BatchNorm layers are analytically fused into the preceding Conv weights and biases:
$$\mathbf{W}_{\text{fused}} = \mathbf{W} \cdot \frac{\gamma}{\sqrt{\sigma^2 + \epsilon}}, \quad \mathbf{b}_{\text{fused}} = (\mathbf{b} - \mu) \cdot \frac{\gamma}{\sqrt{\sigma^2 + \epsilon}} + \beta$$

The payload contains exactly the following layers in sequence:

1. **Input Convolution (`conv_in`)**:
   - `convInW`: float32 array of shape `[channels, 18, 3, 3]` ($C \times 18 \times 9$ floats)
   - `convInB`: float32 array of shape `[channels]` ($C$ floats)
2. **Residual Blocks (`blocks[0..num_blocks-1]`)**:
   For each block $b \in [0, \text{num\_blocks}-1]$:
   - `conv1W`: float32 array of shape `[channels, channels, 3, 3]` ($C \times C \times 9$ floats)
   - `conv1B`: float32 array of shape `[channels]` ($C$ floats)
   - `conv2W`: float32 array of shape `[channels, channels, 3, 3]` ($C \times C \times 9$ floats)
   - `conv2B`: float32 array of shape `[channels]` ($C$ floats)
3. **Policy Head**:
   - `policyConvW`: float32 array of shape `[16, channels, 1, 1]` ($16 \times C$ floats)
   - `policyConvB`: float32 array of shape `[16]` ($16$ floats)
   - `policyFcW`: float32 array of shape `[num_actions, 16 * 64]` ($4240 \times 1024$ floats)
   - `policyFcB`: float32 array of shape `[num_actions]` ($4240$ floats)
4. **Value Head**:
   - `valueConvW`: float32 array of shape `[4, channels, 1, 1]` ($4 \times C$ floats)
   - `valueConvB`: float32 array of shape `[4]` ($4$ floats)
   - `valueFc1W`: float32 array of shape `[64, 4 * 64]` ($64 \times 256$ floats)
   - `valueFc1B`: float32 array of shape `[64]` ($64$ floats)
   - `valueFc2W`: float32 array of shape `[1, 64]` ($64$ floats)
   - `valueFc2B`: float32 array of shape `[1]` ($1$ float)

Total number of floats:
$$N_{\text{floats}} = (162C + C) + B \cdot (18C^2 + 2C) + (16C + 16 + 4240 \cdot 1024 + 4240) + (4C + 4 + 16384 + 64 + 64 + 1)$$
Expected `payload_bytes` $= N_{\text{floats}} \times 4$.

---

## 6. SHA-256 Integrity Checksum

Immediately following the float32 weights payload are 32 bytes of raw binary SHA-256 digest computed over:
$$\text{Digest} = \text{SHA-256}(\text{Header}[0..63] \mathbin{\Vert} \text{Metadata Payload} \mathbin{\Vert} \text{Weights Payload})$$

The loader computes SHA-256 incrementally over these bytes and strictly rejects the file if the computed digest does not match the trailing 32 bytes.

---

## 7. Validation Bounds & Error Rejection

The C++ loader enforces:
1. File size must equal $64 + \text{meta\_bytes} + \text{payload\_bytes} + 32$. Any discrepancy (truncation or trailing bytes) is rejected.
2. `magic` must match `ROOKSAC2`.
3. `format_version` must equal `2`.
4. Bounds:
   - `input_channels` $= 18$
   - `num_blocks` $\in [1, 32]$
   - `channels` $\in [8, 512]$
   - `num_actions` $= 4240$
5. `payload_bytes` must exactly equal $N_{\text{floats}} \times 4$.
6. Weights verification: No float value may be NaN or Inf (`std::isnan`, `std::isinf`).
7. Checksum mismatch fails model loading immediately.

