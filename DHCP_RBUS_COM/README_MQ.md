POSIX Message Queue usage for DHCP Manager

Overview
- `DHCP_RBUS_COM/dhcpmgr_rbus_apis.c` now sends `DhcpManagerEvent` values (as binary ints) to a POSIX message queue named by `MQ_NAME` (default: "/lan_fsm_queue").
- `SM_DHCPMGR/mq_consumer.c` is a small example program that opens the queue, receives `DhcpManagerEvent` messages and calls `DispatchDHCP_SM(evt, NULL)`.

Notes/requirements
- Message size: the queue is created with `mq_msgsize = sizeof(DhcpManagerEvent)` (typically sizeof(int)). Ensure this matches consumer expectations.
- Permissions: the queue is created with mode `0666`. If processes run under different users, ensure permissions are correct.
- Creation semantics: sender attempts an `mq_open(MQ_NAME, O_WRONLY)`. If the queue is missing, the sender will create it with `O_CREAT` and the attributes specified (mq_maxmsg=10, mq_msgsize=sizeof(DhcpManagerEvent)).
- Consumer behavior: `mq_consumer.c` creates/opens the queue with `O_RDONLY | O_CREAT` and unlinks the queue on exit.

How to build the consumer example
- Example compile (from repository root):

```sh
gcc -o SM_DHCPMGR/mq_consumer SM_DHCPMGR/mq_consumer.c -lrt
```

(You may need `-I` include paths if headers are not in default locations.)

How to run
1. Start the consumer (it will create the queue if missing):

```sh
./SM_DHCPMGR/mq_consumer
```

2. Trigger events (via RBUS or other control) — the sender will write `DhcpManagerEvent` integers to the queue.

3. Stop consumer with Ctrl-C. The example unlinks the queue on clean exit.

Next steps / options
- Add the consumer to the SM build system or run it as part of the DHCP Manager service.
- Add robust error handling/retries and health-checks in production code.
- If you want the sender to never create the queue, I can switch the sender to only open and return an error when queue is missing.
