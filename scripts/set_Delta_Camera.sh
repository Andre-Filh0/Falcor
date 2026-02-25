#!/bin/bash

# Configurações
INTERFACE="eth1"
KRIA_IP="192.168.137.101/24"
CAMERA_IP="192.168.137.100"

echo "--- Iniciando configuração da rede para Câmera DMV ---"

# 1. Levanta a interface e limpa IPs antigos
sudo ip link set $INTERFACE up
sudo ip addr flush dev $INTERFACE

# 2. Configura o novo IP na sub-rede 137
echo "Configurando IP $KRIA_IP na interface $INTERFACE..."
sudo ip addr add $KRIA_IP dev $INTERFACE

# 3. Ajustes de Performance e Descoberta (Otimização para GigE Vision)
# Desativa o filtro de caminho reverso (essencial para descoberta de câmeras)
sudo sysctl -w net.ipv4.conf.$INTERFACE.rp_filter=0 > /dev/null
sudo sysctl -w net.ipv4.conf.all.rp_filter=0 > /dev/null

# Aumenta os buffers de rede (evita perda de frames em alta resolução)
sudo sysctl -w net.core.rmem_max=33554432 > /dev/null
sudo sysctl -w net.core.rmem_default=33554432 > /dev/null

echo "Ajustes de sistema aplicados."

# 4. Verificação física e lógica
echo "Aguardando link e testando ping para $CAMERA_IP..."
sleep 2

if ping -c 2 $CAMERA_IP > /dev/null; then
    echo "✅ SUCESSO: Câmera encontrada e acessível!"
    echo "Agora você pode rodar: ./Falcor"
else
    echo "❌ ERRO: Não foi possível dar ping na câmera."
    echo "Verifique os cabos e se a câmera está ligada."
fi