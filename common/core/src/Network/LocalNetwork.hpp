#pragma once

#include <zmq.hpp>
#include <string>

// 1. Forward Declaration: Avisa ao compilador que Socket é um template
template <typename TBuffer> class Socket;

class LocalNetwork {
public:
    // Deixamos o Contexto estático e público para ser acessado pelo Socket
    static inline zmq::context_t Context{1};
private:
    // 2. Declaração de Amizade correta para Templates
    template <typename T> friend class Socket;
    
    // Construtor privado para garantir o padrão Singleton ou Utility
    LocalNetwork() = default;
};