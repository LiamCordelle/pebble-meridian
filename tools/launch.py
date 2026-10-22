"""Launch this project's UUID explicitly; run with pebble-tool's Python."""
import json
import time
from pathlib import Path
from uuid import UUID
from libpebble2.communication import PebbleConnection
from libpebble2.protocol.apps import AppRunState,AppRunStateStart,AppRunStateRequest
from pebble_tool.sdk.emulator import ManagedEmulatorTransport
root=Path(__file__).resolve().parents[1]
uuid=UUID(json.loads((root/'package.json').read_text())['pebble']['uuid'])
connection=PebbleConnection(ManagedEmulatorTransport('emery'))
connection.connect()
connection.run_async()
connection.send_packet(AppRunState(data=AppRunStateStart(uuid=uuid)))
time.sleep(2)
state=connection.send_and_read(AppRunState(data=AppRunStateRequest()),AppRunState)
assert state.data.uuid==uuid,('Unexpected active UUID',state.data.uuid,uuid)
connection.transport.ws.close()
print('Requested launch of',uuid)
