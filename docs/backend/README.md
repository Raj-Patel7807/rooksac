# Backend

No backend exists. A future backend belongs under `app/backend/` and is a separate product layer. It may orchestrate engine processes through a documented UCI/process boundary; it must not import or depend on engine internals.

Databases, queues, authentication, and network services are deferred.
