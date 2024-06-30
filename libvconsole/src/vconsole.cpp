#include "vconsole.h"
#include <iostream>
#include <cstring>

VConsole::VConsole() : clientSocket(INVALID_SOCKET)
{
    setupWinsock();
}

VConsole::~VConsole()
{
    disconnect();
}

bool VConsole::connect(const std::string& ip, int port)
{
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET)
    {
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);

    if (::connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
        return false;
    }

    return true;
}

void VConsole::disconnect()
{
    if (clientSocket != INVALID_SOCKET)
    {
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
    }
    if (onDisconnected)
    {
        onDisconnected();
    }
}

void VConsole::sendCmd(const std::string& cmd)
{
    std::vector<unsigned char> payload = createCommandPayload(cmd);
    send(clientSocket, reinterpret_cast<const char*>(payload.data()), static_cast<int>(payload.size()), 0);
}

int VConsole::readChunk(std::vector<char>& outputBuf)
{
    VConChunk header;
    int n = recv(clientSocket, reinterpret_cast<char*>(&header), sizeof(header), 0);
    if (n < static_cast<int>(sizeof(VConChunk)))
    {
        if (WSAGetLastError() == WSAEWOULDBLOCK)
        {
            return 0;
        }
        return -1;
    }

    header.version = ntohl(header.version);
    header.length = ntohs(header.length);
    header.handle = ntohs(header.handle);

    outputBuf.resize(header.length);
    memcpy(outputBuf.data(), &header, sizeof(VConChunk));

    int p = recv(clientSocket, outputBuf.data() + sizeof(VConChunk), header.length - sizeof(VConChunk), 0);
    if (p < static_cast<int>(header.length - sizeof(VConChunk)))
    {
        if (WSAGetLastError() == WSAEWOULDBLOCK)
        {
            return 0;
        }
        return -1;
    }

    return 1;
}

void VConsole::processIncomingData()
{
    std::vector<char> chunkBuf;
    while (true)
    {
        int result = readChunk(chunkBuf);
        if (result <= 0)
        {
            if (result < 0)
            {
            }
            break;
        }

        VConChunk* header = reinterpret_cast<VConChunk*>(chunkBuf.data());
        std::string msgType(header->type, 4);
        // spdlog::debug("[libvconsole] Processing message type: {}, version: {}, length: {}, handle: {}",
        //               msgType, header->version, header->length, header->handle);
        processPacket(msgType, chunkBuf);
    }
}

void VConsole::processPacket(const std::string& msgType, const std::vector<char>& chunkBuf)
{
    if (msgType == "AINF")
    {
        AINF ainf = parseAINF(chunkBuf);
    }
    else if (msgType == "ADON")
    {
        ADON adon = parseADON(chunkBuf);
        adonName = adon.name;
        if (onADONReceived)
        {
            onADONReceived(adonName);
        }
    }
    else if (msgType == "CHAN")
    {
        CHAN chan = parseCHAN(chunkBuf);
        if (onCHANReceived)
        {
            onCHANReceived(chan);
        }
    }
    else if (msgType == "PRNT")
    {
        PRNT prnt = parsePRNT(chunkBuf);
        prnt.message = stripNonAscii(prnt.message);

        if (onPRNTReceived)
        {
            onPRNTReceived(prnt);
        }
    }
    else if (msgType == "CVAR")
    {
        CVAR cvar = parseCVAR(chunkBuf);
        cvars.push_back(cvar.cvar);
        if (onCVARsLoaded)
        {
            onCVARsLoaded(cvars);
        }
    }
    else if (msgType == "CFGV")
    {
        CFGV cfgv = parseCFGV(chunkBuf);
    }
    else
    {
    }
}

void VConsole::setOnPRNTReceived(std::function<void(const PRNT&)> callback)
{
    onPRNTReceived = callback;
}

void VConsole::setOnCVARsLoaded(std::function<void(const std::vector<Cvar>&)> callback)
{
    onCVARsLoaded = callback;
}

void VConsole::setOnADONReceived(std::function<void(const std::string&)> callback)
{
    onADONReceived = callback;
}

void VConsole::setOnDisconnected(std::function<void()> callback)
{
    onDisconnected = callback;
}

void VConsole::setOnCHANReceived(std::function<void(const CHAN&)> callback)
{
    onCHANReceived = callback;
}

bool setupWinsock()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        return false;
    }
    return true;
}

std::vector<unsigned char> createCommandPayload(const std::string& command)
{
    std::vector<unsigned char> payload = {'C', 'M', 'N', 'D', 0x00, 0xD4, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00};

    size_t maxCommandLength = 65535 - payload.size() - 1;
    std::string truncatedCommand = command.substr(0, maxCommandLength);

    payload.insert(payload.end(), truncatedCommand.begin(), truncatedCommand.end());
    payload.push_back(0x00);

    unsigned short totalLength = static_cast<unsigned short>(payload.size());
    payload[8] = (totalLength >> 8) & 0xFF;
    payload[9] = totalLength & 0xFF;

    return payload;
}

AINF VConsole::parseAINF(const std::vector<char>& chunkBuf)
{
    AINF ainf;
    const uint32_t* data = reinterpret_cast<const uint32_t*>(chunkBuf.data() + sizeof(VConChunk));
    ainf.unknowns.assign(data, data + 19);
    ainf.padding = chunkBuf[sizeof(VConChunk) + 19 * sizeof(uint32_t)];
    return ainf;
}

ADON VConsole::parseADON(const std::vector<char>& chunkBuf)
{
    ADON adon;
    const char* data = chunkBuf.data() + sizeof(VConChunk);
    adon.unknown = ntohs(*reinterpret_cast<const uint16_t*>(data));
    adon.nameLength = ntohs(*reinterpret_cast<const uint16_t*>(data + 2));
    adon.name = std::string(data + 4, adon.nameLength);
    return adon;
}

CHAN VConsole::parseCHAN(const std::vector<char>& chunkBuf)
{
    CHAN chan;
    const char* data = chunkBuf.data() + sizeof(VConChunk);

    size_t remainingSize = chunkBuf.size() - sizeof(VConChunk) - sizeof(uint16_t);
    chan.numChannels = remainingSize / sizeof(Channel);


    data += sizeof(uint16_t);


    chan.channels.reserve(chan.numChannels);
    for (int i = 0; i < chan.numChannels; ++i)
    {
        Channel channel;
        channel.id = ntohl(*reinterpret_cast<const int32_t*>(data));
        data += sizeof(int32_t);

        channel.unknown1 = ntohl(*reinterpret_cast<const int32_t*>(data));
        data += sizeof(int32_t);

        channel.unknown2 = ntohl(*reinterpret_cast<const int32_t*>(data));
        data += sizeof(int32_t);

        channel.verbosity_default = ntohl(*reinterpret_cast<const int32_t*>(data));
        data += sizeof(int32_t);

        channel.verbosity_current = ntohl(*reinterpret_cast<const int32_t*>(data));
        data += sizeof(int32_t);

        channel.text_RGBA_override = ntohl(*reinterpret_cast<const uint32_t*>(data));
        data += sizeof(uint32_t);

        memcpy(channel.name, data, sizeof(channel.name));
        channel.name[sizeof(channel.name) - 1] = '\0'; // Ensure null termination
        data += sizeof(channel.name);

        chan.channels.push_back(channel);
    }

    return chan;
}

PRNT VConsole::parsePRNT(const std::vector<char>& chunkBuf)
{
    PRNT prnt;
    const char* data = chunkBuf.data() + sizeof(VConChunk);
    prnt.channelID = ntohl(*reinterpret_cast<const int32_t*>(data));
    memcpy(prnt.unknown, data + 4, 24);
    prnt.message = std::string(data + 28);
    prnt.message = stripNonAscii(prnt.message); // Strip non-ASCII characters
    return prnt;
}

CVAR VConsole::parseCVAR(const std::vector<char>& chunkBuf)
{
    CVAR cvar;
    memcpy(&cvar.cvar, chunkBuf.data() + sizeof(VConChunk), sizeof(Cvar));
    cvar.cvar.unknown = ntohl(cvar.cvar.unknown);
    cvar.cvar.flags = ntohl(cvar.cvar.flags);

    uint32_t temp;
    memcpy(&temp, &cvar.cvar.rangemin, sizeof(temp));
    temp = ntohl(temp);
    memcpy(&cvar.cvar.rangemin, &temp, sizeof(cvar.cvar.rangemin));

    memcpy(&temp, &cvar.cvar.rangemax, sizeof(temp));
    temp = ntohl(temp);
    memcpy(&cvar.cvar.rangemax, &temp, sizeof(cvar.cvar.rangemax));

    return cvar;
}

CFGV VConsole::parseCFGV(const std::vector<char>& chunkBuf)
{
    CFGV cfgv;
    memcpy(&cfgv, chunkBuf.data() + sizeof(VConChunk), sizeof(CFGV));
    return cfgv;
}
