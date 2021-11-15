import canopen

network = canopen.Network()

node = canopen.RemoteNode(5, 'test.eds')
network.add_node(node)
network.connect(bustype='socketcan', channel='vcan0')

node.tpdo.read()
# Re-map TxPDO1
node.tpdo[1].clear()
node.tpdo[1].add_variable('Test 1')
node.tpdo[1].add_variable('Test 2')
node.tpdo[1].trans_type = 0xFF
node.tpdo[1].event_timer = 500
node.tpdo[1].enabled = True

# Save new PDO configuration to node
node.tpdo.save()

while True:
    print("TPDO1 received")
    node.tpdo[1].wait_for_reception()
    print("\tTest value 1:", node.tpdo[1]['Test 1'].raw)
    print("\tTest value 2:", node.tpdo[1]['Test 2'].raw)
