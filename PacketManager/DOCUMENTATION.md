## pckt::Type
Defines the "types" of packets that can exist, that being *None*, *DataPacket*, and *AckPacket*, user can define a callback for any one of these
<br>

## pckt::Packet
Defines the structure that packets take, and has the following fields
* *uint8_t* magic - The magic number used to search for a packet
* *uint8_t* type - The type of packet this is
* *uint8_t* flags - Contains both user defined and custom control flags
* *uint8_t[MAX_PAYLOAD_SIZE]* payload - The user-defined payload for the packet
* *uint16_t* checksum - Simple checksum used to validate the packet
<br>

## pckt::Transport
An abstract interface to read/write data to some buffer or stream
<br>

## pckt::PacketManager
Manages recieving / sending packets via some transport

#### void PacketManager.Update()
Handles the main logic for the packet manager, transport is expected to have been provided by this point. If a packet is recieved and is validated, the PacketManager will call the associated callback function if one was set.

#### void PacketManager.Callback(Type type, Handler handler)
Sets the callback function to use when a packet of type is recieved

#### void PacketManager.Send(Type type, const uint8_t* payload, size_t len);
Sends the packet with the provided payload and type over transport, if len is greater than MAX_PAYLOAD_SIZE then only MAX_PAYLOAD_SIZE bytes are sent

#### static bool PacketManager.HasFlag<uint8_t idx>(const Packet& packet)
Returns true if the packet has the flag bit at the provided idx set, idx must be [0, 3] and will fail to compile if otherwise

#### void PacketManager.SetFlag<uint8_t idx>(bool v)
Sets the flag bit at idx to 1 if v is true, otherwise sets the flag bit at idx to 0, idx must be [0,3] and will fail to compile if otherwise

#### void PacketManager.SetCritical(bool v)
Sets the critical bit to 1 if v is true, otherwise sets it to 0, currently the critical bit has no impact
<br>

## trns::SerialTransport
Provides the implementation for pckt::Transport for the SoftwareSerial stream
<br>
