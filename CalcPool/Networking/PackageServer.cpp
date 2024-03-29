// Philipp Neufeld, 2023

#include <algorithm>
#include <cstdlib>
#include <sstream>

#include <arpa/inet.h> // htonl, ntohl

#include "PackageServer.h"

// https://stackoverflow.com/questions/3022552/is-there-any-standard-htonl-like-function-for-64-bits-integers-in-c
#ifdef __BIG_ENDIAN__
#   define htonll(x) (x)
#   define ntohll(x) (x)
#else
#   define htonll(x) (((std::uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#   define ntohll(x) (((std::uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))
#endif


namespace CP
{

    //
    // DataPackagePayload
    //

    DataPackagePayload::DataPackagePayload() 
        : m_size(0), m_pData(nullptr) {}

    DataPackagePayload::DataPackagePayload(std::uint64_t size) 
        : DataPackagePayload() 
    {
        Allocate(size);
    }
    
    DataPackagePayload::~DataPackagePayload() 
    { 
        Allocate(0);
    }

    DataPackagePayload::DataPackagePayload(const DataPackagePayload& rhs)
        : DataPackagePayload()
    {
        if (rhs.m_size > 0)
        {
            Allocate(rhs.m_size);
            std::copy(rhs.m_pData, rhs.m_pData+rhs.m_size, m_pData);
        }
    }
    
    DataPackagePayload::DataPackagePayload(DataPackagePayload&& rhs)
        : m_pData(rhs.m_pData), m_size(rhs.m_size)
    {
        rhs.m_pData = nullptr;
        rhs.m_size = 0;
    }

    DataPackagePayload& DataPackagePayload::operator=(const DataPackagePayload& rhs)
    {
        DataPackagePayload tmp(rhs);
        std::swap(*this, tmp);
        return *this;
    }

    DataPackagePayload& DataPackagePayload::operator=(DataPackagePayload&& rhs)
    {
        std::swap(m_pData, rhs.m_pData);
        std::swap(m_size, rhs.m_size);
        return *this;
    }
    
    bool DataPackagePayload::Allocate(std::uint64_t size)
    {
        if (size == m_size)
            return true;

        if (m_pData)
            delete[] m_pData;
        m_pData = nullptr;
        m_size = 0;      
        
        if (size > 0)
        {
            m_pData = new std::uint8_t[size];
            m_size = size;
            if (!m_pData)
            {
                m_pData = nullptr;
                m_size = 0;
            }
        }

        return (size == m_size);
    }

    //
    // NetworkDataPackage
    //

    NetworkDataPackage::NetworkDataPackage() 
        : DataPackagePayload() {}

    NetworkDataPackage::NetworkDataPackage(std::uint64_t size) 
        : DataPackagePayload(size)
    {
        Allocate(size);
    }

    NetworkDataPackage::NetworkDataPackage(const Header_t& header)
        : NetworkDataPackage()
    {
        if (IsValidHeader(header))
        {
            m_messageId = GetMessageIdFromHeader(header);
            m_topic = GetTopicFromHeader(header);
            Allocate(GetSizeFromHeader(header));
        }
    }

    NetworkDataPackage::NetworkDataPackage(const NetworkDataPackage& rhs)
        : DataPackagePayload(rhs), m_messageId(rhs.m_messageId), m_topic(rhs.m_topic) {}
    
    NetworkDataPackage::NetworkDataPackage(NetworkDataPackage&& rhs)
        : DataPackagePayload(std::move(rhs)), m_messageId(rhs.m_messageId), 
        m_topic(std::move(rhs.m_topic)) {}

    NetworkDataPackage::NetworkDataPackage(const DataPackagePayload& rhs)
        : DataPackagePayload(rhs) {}
    
    NetworkDataPackage::NetworkDataPackage(DataPackagePayload&& rhs)
        : DataPackagePayload(std::move(rhs)) {}

    NetworkDataPackage& NetworkDataPackage::operator=(const NetworkDataPackage& rhs)
    {
        NetworkDataPackage tmp(rhs);
        std::swap(*this, tmp);
        return *this;
    }

    NetworkDataPackage& NetworkDataPackage::operator=(NetworkDataPackage&& rhs)
    {
        DataPackagePayload::operator=(std::move(rhs));
        m_messageId = rhs.m_messageId;
        m_topic = std::move(rhs.m_topic);
        return *this;
    }

    NetworkDataPackage& NetworkDataPackage::operator=(const DataPackagePayload& rhs)
    {
        DataPackagePayload::operator=(rhs);
        return *this;
    }

    NetworkDataPackage& NetworkDataPackage::operator=(DataPackagePayload&& rhs)
    {
        DataPackagePayload::operator=(std::move(rhs));
        return *this;
    }
    
    bool NetworkDataPackage::IsValidHeader(const Header_t& header)
    {
        std::uint64_t npid = 0;
        std::copy_n(header.data() + s_pidOff, 8, reinterpret_cast<std::uint8_t*>(&npid));
        std::uint64_t pid = ntohll(npid);
        return (pid == s_protocolId);
    }
    
    std::uint64_t NetworkDataPackage::GetSizeFromHeader(const Header_t& header)
    {
        std::uint64_t nsize = 0;
        std::copy_n(header.data() + s_sizeOff, sizeof(nsize), reinterpret_cast<std::uint8_t*>(&nsize));
        return ntohll(nsize);
    }

    std::uint32_t NetworkDataPackage::GetMessageIdFromHeader(const Header_t& header)
    {
        std::uint32_t nmid = 0;
        std::copy_n(header.data() + s_msgOff, sizeof(nmid), reinterpret_cast<std::uint8_t*>(&nmid));
        return ntohl(nmid);
    }
    
    UUIDv4 NetworkDataPackage::GetTopicFromHeader(const Header_t& header)
    {
        return UUIDv4::LoadFromBufferLE(header.data() + s_topicOff);
    }

    NetworkDataPackage::Header_t NetworkDataPackage::GenerateHeader(
        std::uint64_t size, std::uint32_t msgId, const UUIDv4& topic)
    {
        Header_t header;

        std::uint64_t npid = htonll(s_protocolId);
        std::uint64_t nsize = htonll(size);
        std::uint32_t nmid = htonl(msgId);
        
        std::copy_n(reinterpret_cast<const std::uint8_t*>(&npid), 8, header.data() + s_pidOff);
        std::copy_n(reinterpret_cast<const std::uint8_t*>(&nsize), 8, header.data() + s_sizeOff);
        std::copy_n(reinterpret_cast<const std::uint8_t*>(&nmid), 4, header.data() + s_msgOff);
        topic.StoreToBufferLE(header.data() + s_topicOff);

        return header;
    }

    NetworkDataPackage::Header_t NetworkDataPackage::GetHeader() const
    {
        return GenerateHeader(GetSize(), m_messageId, m_topic);
    }

    NetworkDataPackage NetworkDataPackage::CreateEmptyPackage(std::uint32_t msgId)
    {
        NetworkDataPackage package;
        package.SetMessageId(msgId);
        return package;
    }
    

    //
    // PackageConnectionHandler
    //

    PackageConnectionHandler::PackageConnectionHandler() { }
    
    PackageConnectionHandler::~PackageConnectionHandler() 
    {
        while (!m_connections.empty())
            RemoveConnection(*m_connections.begin());

        while (!m_servers.empty())
            RemoveServer(m_servers.front());
    }

    void PackageConnectionHandler::AddConnection(TCPIPConnection* pConn)
    {
        if (!pConn)
            return;
        
        std::unique_lock<std::mutex> lock(m_mutex);
        auto it = m_connections.find(pConn);
        if (it == m_connections.end())
        {
            m_connections.insert(pConn);
            m_wakeupSignal.Set();
        }
    }

    void PackageConnectionHandler::RemoveConnection(TCPIPConnection* pConn)
    {   
        std::unique_lock<std::mutex> lock(m_mutex);
        auto it = m_connections.find(pConn);
        if (it != m_connections.end())
        {
            m_connections.erase(it);

            // clear write buffer from pConn
            auto it2 = m_writeBuffer.find(pConn);
            if (it2 != m_writeBuffer.end())
                m_writeBuffer.erase(it2);

            m_wakeupSignal.Set();

            lock.unlock();
            OnConnectionRemoved(pConn);
        }
    }

    void PackageConnectionHandler::AddServer(TCPIPServerSocket* pServer)
    {
        if (!pServer)
            return;
        
        std::unique_lock<std::mutex> lock(m_mutex);
        auto it = std::find(m_servers.begin(), m_servers.end(), pServer);
        if (it == m_servers.end())
        {
            m_servers.push_back(pServer);
            m_wakeupSignal.Set();
        }
    }

    void PackageConnectionHandler::RemoveServer(TCPIPServerSocket* pServer)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        auto it = std::find(m_servers.begin(), m_servers.end(), pServer);
        if (it != m_servers.end())
        {
            m_servers.erase(it);
            m_wakeupSignal.Set();
        }
    }

    void PackageConnectionHandler::Broadcast(NetworkDataPackage msg)
    {
        for (TCPIPConnection* pConn: m_connections)
            WriteTo(pConn, msg);
    }

    void PackageConnectionHandler::WriteTo(TCPIPConnection* pConn, NetworkDataPackage msg)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_connections.count(pConn) != 0)
        {
            m_writeBuffer[pConn].emplace(std::move(msg), 0ull);
            m_wakeupSignal.Set();
        }
    }

    void PackageConnectionHandler::RunHandler()
    {
        std::map<TCPIPConnection*, std::tuple<NetworkDataPackage::Header_t, NetworkDataPackage, std::uint64_t>> readBuffer;
        std::set<TCPIPConnection*> killed;

        std::vector<TCPIPServerSocket*> checkAcceptable;
        std::vector<TCPIPConnection*> checkReadable;
        std::vector<TCPIPConnection*> checkWritable;

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_keepRunning = true;
        }

        while (true)
        { 
            checkAcceptable.clear();
            checkReadable.clear();
            checkWritable.clear();
            killed.clear();
 
            // prepare lists of connections to be checked
            {
                std::unique_lock<std::mutex> lock(m_mutex);

                if (!m_keepRunning)
                    break;
                m_wakeupSignal.Reset();

                checkAcceptable.reserve(m_servers.size());
                checkReadable.reserve(m_connections.size());
                checkWritable.reserve(m_writeBuffer.size());
                std::copy(m_servers.begin(), m_servers.end(), std::back_inserter(checkAcceptable));
                std::copy(m_connections.begin(), m_connections.end(), std::back_inserter(checkReadable));
                std::transform(m_writeBuffer.begin(), m_writeBuffer.end(), 
                    std::back_inserter(checkWritable), [](auto el) { return el.first; });
            }

            // block until either accept, read or write can be performed
            auto [ac, re, wr, ex] = TCPIPServerSocket::Select(checkAcceptable, checkReadable, checkWritable, {}, m_wakeupSignal);
            
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                if (!m_keepRunning)
                    break;
            }

            // handle new connections
            for (TCPIPServerSocket* pServer: ac)
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                bool found = (std::find(m_servers.begin(), m_servers.end(), pServer) != m_servers.end());
                lock.unlock();
                if (!found)
                    continue;

                this->OnConnectionAcceptale(pServer);
            }

            // validate read buffer
            for (auto it = readBuffer.begin(); it != readBuffer.end();)
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                if (m_connections.count(it->first) == 0)
                    it = readBuffer.erase(it);
                else
                    it = std::next(it);
            }

            // handle receiving messages
            for (TCPIPConnection* pConn: re)
            {
                if (killed.count(pConn) != 0)
                    continue;

                if (m_connections.count(pConn) == 0)
                    continue;
                
                // check if data in read buffer
                if (readBuffer.count(pConn) == 0)
                        readBuffer[pConn] = std::make_tuple(NetworkDataPackage::Header_t{}, NetworkDataPackage{}, std::uint64_t(0));
                auto& [header, buffer, alreadyRead] = readBuffer[pConn];

                if (alreadyRead < sizeof(header))
                {
                    auto cnt = pConn->RecvNonBlock(header.data() + alreadyRead, sizeof(header) - alreadyRead);
                    if (cnt <= 0)
                    {
                        killed.insert(pConn);
                        continue;
                    }
                    alreadyRead += cnt;

                    // check if header is complete and allocate package buffer in that case
                    if (alreadyRead == sizeof(header))
                    {
                        if (!NetworkDataPackage::IsValidHeader(header))
                        {
                            killed.insert(pConn);
                            continue;
                        }
                        buffer = NetworkDataPackage(header);
                    }
                }
                else 
                {
                    std::uint64_t pdataRead = alreadyRead - sizeof(header);
                    std::uint64_t remaining = buffer.GetSize() - pdataRead;
                    auto cnt = pConn->RecvNonBlock(buffer.GetData() + pdataRead, remaining);
                    if (cnt <= 0 || cnt > remaining)
                    {
                        killed.insert(pConn);
                        continue;
                    }
                    alreadyRead += cnt; 
                }

                // package complete?
                if (alreadyRead == (buffer.GetSize() + sizeof(header)))
                {
                    NetworkDataPackage msg = std::move(buffer);
                    readBuffer.erase(readBuffer.find(pConn));
                    OnConnectionMessage(pConn, std::move(msg)); 
                }
            }

            // handle sending messages
            for (TCPIPConnection* pConn: wr)
            {
                if (killed.count(pConn) != 0)
                    continue;
                
                if (m_connections.count(pConn) == 0)
                    continue;

                std::unique_lock<std::mutex> lock(m_mutex);
                auto& outputQueue = m_writeBuffer[pConn];
                if (outputQueue.empty())
                    continue;
                auto& [package, alreadySent] = outputQueue.front();
                lock.unlock();

                // access to write buffer is thread-safe without lock in this case,
                // since all other threads may only append to the list 
                // (this does not invalidate the iterators to the existing items)

                constexpr auto headerSize = sizeof(NetworkDataPackage::Header_t);
                if (alreadySent < headerSize)
                {
                    NetworkDataPackage::Header_t header = package.GetHeader();
                    std::uint64_t remaining = headerSize - alreadySent;
                    auto cnt = pConn->SendNonBlock(header.data() + alreadySent, remaining);
                    if (cnt <= 0 || cnt > remaining)
                    {
                        killed.insert(pConn);
                        continue;
                    }
                    alreadySent += cnt;
                }
                else 
                {
                    std::uint64_t pdataSent = alreadySent - headerSize;
                    std::uint64_t remaining = package.GetSize() - pdataSent;
                    auto cnt = pConn->SendNonBlock(package.GetData() + pdataSent, remaining);
                    if (cnt <= 0 || cnt > remaining)
                    {
                        killed.insert(pConn);
                        continue;
                    }
                    alreadySent += cnt;
                }

                // if sent completely remove package from write buffer
                if (alreadySent == (headerSize + package.GetSize()))
                {
                    lock.lock();
                    outputQueue.pop();
                }
            }
            
            // purge write buffer of empty queues
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                for (auto it = m_writeBuffer.begin(); it != m_writeBuffer.end();)
                {
                    if (it->second.empty())
                        it = m_writeBuffer.erase(it);
                    else
                        it = std::next(it);
                }
                
            }

            // handle killed connections
            for (TCPIPConnection* pConn: killed)
            {
                pConn->Close();
                this->OnConnectionClosed(pConn);
                this->RemoveConnection(pConn);
                
                // clear read buffer
                auto it1 = readBuffer.find(pConn);
                if (it1 != readBuffer.end())
                    readBuffer.erase(it1);

                // clear write buffer
                std::unique_lock<std::mutex> lock(m_mutex);
                auto it2 = m_writeBuffer.find(pConn);
                if (it2 != m_writeBuffer.end())
                    m_writeBuffer.erase(it2);
            }
        }

        // cleanup
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            while (!m_connections.empty())
            {
                TCPIPConnection* pConn = *m_connections.begin();
                pConn->Close();

                lock.unlock();
                this->RemoveConnection(pConn);
                lock.lock();
            }

            m_writeBuffer.clear();
        }
    }

    void PackageConnectionHandler::StopHandler()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_keepRunning = false;
        m_wakeupSignal.Set();
    }


    //
    // ConnectionUUIDMap
    // 

    UUIDv4 ConnectionUUIDMap::GetIdFromConnection(const TCPIPConnection* pConn) const
    {
        auto it = m_fromConn.find(pConn);
        return it == m_fromConn.end() ? UUIDv4::NullUUID() : it->second;
    }

    TCPIPConnection* ConnectionUUIDMap::GetConnectionFromId(UUIDv4 id) const
    {
        auto it = m_fromUuid.find(id);
        return it == m_fromUuid.end() ? nullptr : it->second;
    }

    UUIDv4 ConnectionUUIDMap::Insert(TCPIPConnection* pConn)
    {
        UUIDv4 uuid = m_fromConn[pConn];
        m_fromUuid[uuid] = pConn;
        return uuid;
    }

    void ConnectionUUIDMap::Remove(UUIDv4 id)
    {
        auto it = m_fromUuid.find(id);
        if (it != m_fromUuid.end())
        {
            auto it2 = m_fromConn.find(it->second);
            if (it2 != m_fromConn.end())
                m_fromConn.erase(it2);
            m_fromUuid.erase(it);
        }
    }
    
    void ConnectionUUIDMap::Remove(const TCPIPConnection* pConn)
    {
        auto it = m_fromConn.find(pConn);
        if (it != m_fromConn.end())
        {
            auto it2 = m_fromUuid.find(it->second);
            if (it2 != m_fromUuid.end())
                m_fromUuid.erase(it2);
            m_fromConn.erase(it);
        }
    }


    //
    // PackageServer
    //

    PackageServer::PackageServer() : m_pServer(nullptr) { }
    
    PackageServer::~PackageServer() { }

    bool PackageServer::Run(short port)
    {
        if (m_pServer)
            return false;

        // start socket server
        m_pServer = new TCPIPServerSocket();
        if (!m_pServer->Bind(port))
            return false;
        if (!m_pServer->Listen(5))
            return false;

        this->AddServer(m_pServer);
        this->RunHandler();
        this->RemoveServer(m_pServer);

        delete m_pServer;
        m_pServer = nullptr;

        return true;
    }

    void PackageServer::Stop()
    {
        StopHandler();
    }

    void PackageServer::WriteTo(UUIDv4 id, NetworkDataPackage msg)
    {
        PackageConnectionHandler::WriteTo(m_ids.GetConnectionFromId(id), std::move(msg));
    }

    void PackageServer::Broadcast(NetworkDataPackage msg)
    {
        PackageConnectionHandler::Broadcast(std::move(msg));
    }

    void PackageServer::Disconnect(UUIDv4 id)
    {
        TCPIPConnection* pConn = m_ids.GetConnectionFromId(id);
        pConn->Close();
        OnClientDisconnected(id);
        m_ids.Remove(id);
        PackageConnectionHandler::RemoveConnection(pConn);
    }

    void PackageServer::OnConnectionAcceptale(TCPIPServerSocket* pServer) 
    {
        auto [fd, ip] = pServer->Accept();
        TCPIPConnection* pConn = new TCPIPConnection(fd);
        if (pConn->IsValid())
        {
            AddConnection(pConn);
            UUIDv4 id = m_ids.Insert(pConn);
            OnClientConnected(id, ip);
        }
    }

    void PackageServer::OnConnectionClosed(TCPIPConnection* pConn) 
    {
        OnClientDisconnected(m_ids.GetIdFromConnection(pConn));
    }

    void PackageServer::OnConnectionRemoved(TCPIPConnection* pConn) 
    {
        if (pConn) delete pConn;
    }

    void PackageServer::OnConnectionMessage(TCPIPConnection* pConn, NetworkDataPackage msg) 
    {
        OnMessageReceived(m_ids.GetIdFromConnection(pConn), std::move(msg));
    }


    //
    // PackageClient
    //

    PackageClient::PackageClient() { }

    UUIDv4 PackageClient::Connect(const std::string& ip, short port)
    {
        auto pConn = new TCPIPClientSocket();
        if (!pConn->Connect(ip, port))
        {
            delete pConn;
            return UUIDv4::NullUUID();
        }
        
        AddConnection(pConn);
        return m_ids.Insert(pConn);
    }
    
    UUIDv4 PackageClient::ConnectHostname(const std::string& hostname, short port)
    {
        return Connect(TCPIPClientSocket::GetHostByName(hostname), port);
    }

    void PackageClient::Run()
    {
        RunHandler();
    }

    void PackageClient::Stop()
    {
        StopHandler();
    }

    void PackageClient::WriteTo(UUIDv4 id, NetworkDataPackage data)
    {
        PackageConnectionHandler::WriteTo(m_ids.GetConnectionFromId(id), std::move(data));
    }

    void PackageClient::Disconnect(UUIDv4 id)
    {
        TCPIPConnection* pConn = m_ids.GetConnectionFromId(id);
        pConn->Close();
        OnClientDisconnected(id);
        m_ids.Remove(id);
        PackageConnectionHandler::RemoveConnection(pConn);
    }

    void PackageClient::OnConnectionAcceptale(TCPIPServerSocket* pServer) { }
    
    void PackageClient::OnConnectionClosed(TCPIPConnection* pConn) 
    {
        OnClientDisconnected(m_ids.GetIdFromConnection(pConn));
    }

    void PackageClient::OnConnectionRemoved(TCPIPConnection* pConn) 
    { 
        if (pConn) delete pConn;
    }
    
    void PackageClient::OnConnectionMessage(TCPIPConnection* pConn, NetworkDataPackage msg) 
    {
        OnMessageReceived(m_ids.GetIdFromConnection(pConn), std::move(msg));
    }

}
