#!/bin/bash

# ==============================================================================
# SCRIPT DE INICIALIZAÇÃO E DIAGNÓSTICO DE REDE - VISÃO INDUSTRIAL (GigE)
# Target: Kria KR260 Robotics Starter Kit
# Application: Falcor Pipeline
# ==============================================================================

# 0. Configurações de Cores (Padrão ANSI)
C_RESET='\e[0m'
C_BOLD='\e[1m'
C_RED='\e[31m'
C_GREEN='\e[32m'
C_YELLOW='\e[33m'
C_BLUE='\e[34m'
C_CYAN='\e[36m'

# Parâmetros de Rede
INTERFACE="eth1"
KRIA_IP="192.168.137.101/24"
CAMERA_IP="192.168.137.100"

# Funções de Log Padronizadas
log_info()  { echo -e "${C_CYAN}[INFO]${C_RESET} $1"; }
log_ok()    { echo -e "${C_GREEN}${C_BOLD}[OK]${C_RESET} $1"; }
log_warn()  { echo -e "${C_YELLOW}[WARN]${C_RESET} $1"; }
log_error() { echo -e "${C_RED}${C_BOLD}[ERRO]${C_RESET} $1"; }

# Cabeçalho da Aplicação
echo -e "${C_BLUE}${C_BOLD}"
echo "=========================================================="
echo "         FALCOR VISION SYSTEM - NETWORK BOOTSTRAP         "
echo "==========================================================${C_RESET}"

# 1. Configuração da Interface (Camada Física/Lógica)
log_info "Trazendo interface ${C_BOLD}$INTERFACE${C_RESET} UP e limpando IPs antigos..."
sudo ip link set $INTERFACE up
sudo ip addr flush dev $INTERFACE

# 2. Configuração do IP (Camada de Rede)
log_info "Atribuindo IPv4 Estático: ${C_BOLD}$KRIA_IP${C_RESET} na $INTERFACE..."
sudo ip addr add $KRIA_IP dev $INTERFACE

# 3. Otimizações de Kernel (GigE Vision e Prevenção de Drop de Frames)
log_info "Aplicando otimizações de Kernel (High-Throughput)..."
sudo sysctl -w net.ipv4.conf.$INTERFACE.rp_filter=0 > /dev/null
sudo sysctl -w net.ipv4.conf.all.rp_filter=0 > /dev/null
sudo sysctl -w net.core.rmem_max=33554432 > /dev/null
sudo sysctl -w net.core.rmem_default=33554432 > /dev/null
log_ok "Ajustes de buffers (32MB) e reverse-path filter aplicados."

echo "----------------------------------------------------------"

# 4. Loop de Descoberta da Câmera (Watchdog)
log_info "Iniciando rotina de descoberta no IP alvo: ${C_BOLD}$CAMERA_IP${C_RESET}"

while true; do
    log_info "Enviando requisição ICMP (Ping) para $CAMERA_IP..."
    
    # Ping com timeout de 2 segundos (-W 2) para não travar muito tempo
    if ping -c 2 -W 2 $CAMERA_IP > /dev/null 2>&1; then
        echo ""
        log_ok "Hardware encontrado! Câmera respondendo perfeitamente."
        log_ok "Link de dados GigE estabelecido com sucesso."
        echo -e "${C_GREEN}${C_BOLD}>>> O sistema está pronto. Você já pode iniciar a pipeline. <<<${C_RESET}\n"
        break
    else
        echo ""
        log_error "Timeout. Câmera não detectada na rede."
        log_warn "Verifique a alimentação e os cabos na porta $INTERFACE da KR260."
        log_warn "Pressione '1' durante a contagem para abortar a busca."
        
        echo -ne "${C_YELLOW}Retentando conexão em: ${C_RESET}"
        
        # Contagem regressiva de 5 a 0
        for i in {5..0}; do
            echo -ne "${C_BOLD}$i... ${C_RESET}"
            
            # Lê o teclado por 1 segundo sem travar o loop
            read -t 1 -n 1 input
            if [[ $input == "1" ]]; then
                echo ""
                echo ""
                log_warn "Operação abortada pelo operador (Tecla '1' detectada)."
                log_info "Saindo do script de bootstrap."
                exit 1 # Encerra o script com código de erro (útil se for chamado por outro script)
            fi
        done
        
        echo ""
        log_info "Reiniciando ciclo de varredura..."
        echo "----------------------------------------------------------"
    fi
done

# Aqui você pode chamar o Falcor automaticamente se quiser:
# cd /home/andrefilho/Projects/Falcor/out/bin && sudo ./Falcor