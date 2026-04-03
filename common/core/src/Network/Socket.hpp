#pragma once

#include <zmq.hpp>
#include <string>
#include <vector>
#include <filesystem>
#include <optional>
#include "Logger.hpp"

namespace fs = std::filesystem;

enum class NetProtocol { TCP, INPROC, IPC };

enum class SocketType : int {
    PUSH = int(zmq::socket_type::push),
    PULL = int(zmq::socket_type::pull),
    PUB  = int(zmq::socket_type::pub),
    SUB  = int(zmq::socket_type::sub),
    REP  = int(zmq::socket_type::rep),
    REQ  = int(zmq::socket_type::req)
};

struct SocketConfig {
    NetProtocol protocol;
    SocketType socketType;
    std::string port;
    bool isBind = false;      // True = Servidor, False = Cliente
    std::string host = "127.0.0.1"; // NOVO: Para TCP, define o IP alvo (padrão localhost)
};

template <typename TBuffer>
class Socket {
public:
    SocketConfig m_config;
    zmq::context_t& m_ctx;
    zmq::socket_t m_socket;

    Socket(const SocketConfig &config, zmq::context_t &context)
        : m_config(config),
          m_ctx(context),
          m_socket(m_ctx, static_cast<zmq::socket_type>(m_config.socketType))
    {
        start();
    }

    virtual ~Socket() {
        try {
            m_socket.close();
            // Apenas o Servidor IPC limpa o arquivo
            if (m_config.isBind && m_config.protocol == NetProtocol::IPC) {
                std::string path = "/tmp/" + m_config.port + ".ipc";
                if (fs::exists(path)) fs::remove(path);
            }
        } catch (...) {}
    }

    virtual bool sendData(const TBuffer &data) {
        auto res = m_socket.send(zmq::buffer(data), zmq::send_flags::dontwait);
        return res.has_value();
    }

    virtual std::optional<TBuffer> getData(bool waitIfEmpty = false) {
        zmq::message_t msg;
        zmq::recv_flags flags = waitIfEmpty ? zmq::recv_flags::none : zmq::recv_flags::dontwait;

        try {
            auto res = m_socket.recv(msg, flags);
            if (res && res.value() > 0) {
                return TBuffer(static_cast<const char*>(msg.data()), msg.size());
            }
        } catch (const zmq::error_t& e) {
            if (e.num() != EAGAIN)
                Falcor::SysLog.log_error("[Net] Erro em recv(): {}", e.what());
        }
        return std::nullopt;
    }

private:
    void start() {
        std::string addr;
        std::string ipcPath = "/tmp/" + m_config.port + ".ipc";

        // Lógica Universal de Endereçamento
        if (m_config.protocol == NetProtocol::IPC) {
            addr = "ipc://" + ipcPath;
        } else if (m_config.protocol == NetProtocol::TCP) {
            if (m_config.isBind) {
                addr = "tcp://*:" + m_config.port; // Servidor aceita de qualquer IP
            } else {
                addr = "tcp://" + m_config.host + ":" + m_config.port; // Cliente vai no IP específico
            }
        } else {
            addr = "inproc://" + m_config.port; // Comunicação entre threads (Ultra rápido)
        }

        try {
            if (m_config.isBind) {
                if (m_config.protocol == NetProtocol::IPC && fs::exists(ipcPath))
                    fs::remove(ipcPath); // Limpeza automatica de socket IPC anterior
                m_socket.bind(addr);
                Falcor::SysLog.log_info("[Net] BIND: {}", addr);
            } else {
                m_socket.connect(addr);
                Falcor::SysLog.log_info("[Net] CONNECT: {}", addr);
            }

            if (m_config.socketType == SocketType::SUB)
                m_socket.set(zmq::sockopt::subscribe, "");

        } catch (const zmq::error_t& e) {
            Falcor::SysLog.log_error("[Net] Falha ao iniciar socket ({}): {}", addr, e.what());
        }
    }
};