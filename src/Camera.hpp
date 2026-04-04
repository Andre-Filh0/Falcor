#pragma once

/**
 * @file src/Camera.hpp
 * @brief API de alto nível para câmera Delta GigE Vision no projeto Falcor.
 *
 * Este arquivo expõe as funções de câmera específicas do projeto no namespace
 * Falcor::Delta. A implementação delega para CCameraComponent (retry, threading)
 * e ImplCameraComponent_DMV_SDK (SDK Delta v1.5.4).
 *
 * Regra arquitetural:
 *   common/ = base reutilizável para qualquer projeto embarcado
 *   src/    = código específico do Falcor (aqui fica)
 *
 * Uso típico em main.cpp:
 * @code
 *   auto& cam = ComponentSystem::get<CCameraComponent>();
 *
 *   // Enumera antes de conectar (debug / diagnóstico)
 *   Falcor::Delta::enumerateCameras();
 *
 *   // Após conectar, carrega preset do curral
 *   Falcor::Delta::loadPreset(cam, DeltaUserPreset::UserSet1);
 *
 *   // Salva a configuração atual no preset 2
 *   Falcor::Delta::savePreset(cam, DeltaUserPreset::UserSet2);
 * @endcode
 */

#include "CCameraComponent.hpp"
#include "vendors/ImplCameraComponent_DMV_SDK.hpp"

#include <string>
#include <vector>

namespace Falcor::Delta {

// ─────────────────────────────────────────────────────────────────────────────
// Descoberta de câmeras na rede (sem precisar de IP)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Descobre e loga todas as câmeras Delta na rede.
 *
 * Cria um DcSystem temporário, varre todas as interfaces GigE Vision
 * e loga IP, MAC, modelo e serial de cada câmera encontrada.
 * Não é necessário saber o IP previamente.
 *
 * @param timeoutMs  Tempo de espera por interface (padrão: 2 s).
 * @return           Vetor das câmeras encontradas.
 */
std::vector<DeltaDiscoveredCamera> enumerateCameras(
    uint32_t timeoutMs = 2000);

// ─────────────────────────────────────────────────────────────────────────────
// Gerenciamento de User Presets
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Carrega um User Preset da memória da câmera.
 *
 * Os parâmetros de imagem (exposure, gain, resolução, formato, trigger)
 * são restaurados imediatamente. Câmera deve estar conectada.
 *
 * @param cam     Componente de câmera (já inicializado).
 * @param preset  Preset a carregar (Default = fábrica, UserSet1–4 = usuário).
 * @return        CameraStatus::Ok em sucesso.
 */
CameraStatus loadPreset(CCameraComponent& cam, DeltaUserPreset preset);

/**
 * @brief Salva as configurações atuais em um User Preset.
 *
 * Persiste na memória não-volátil da câmera. Default é somente leitura
 * e será rejeitado (retorna InvalidConfiguration).
 *
 * @param cam     Componente de câmera conectado.
 * @param preset  Slot de destino (UserSet1–4).
 * @return        CameraStatus::Ok em sucesso.
 */
CameraStatus savePreset(CCameraComponent& cam, DeltaUserPreset preset);

/**
 * @brief Define qual preset é carregado automaticamente ao ligar a câmera.
 *
 * Útil para garantir que a câmera sempre inicie com as configurações
 * corretas para o ambiente do curral sem necessidade de intervenção.
 *
 * @param cam     Componente de câmera conectado.
 * @param preset  Preset a usar no boot.
 * @return        CameraStatus::Ok em sucesso.
 */
CameraStatus setBootPreset(CCameraComponent& cam, DeltaUserPreset preset);

// ─────────────────────────────────────────────────────────────────────────────
// Informações da câmera
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Lê e retorna as informações completas da câmera conectada.
 *
 * Popula modelo, serial, IP, MAC, firmware, resolução, FPS, exposure,
 * gain e preset ativo diretamente dos nós GenICam.
 *
 * @param cam  Componente de câmera conectado.
 * @return     Struct preenchida; campos indisponíveis ficam como "N/A" ou 0.
 */
DeltaCameraInfo getCameraInfo(const CCameraComponent& cam);

/**
 * @brief Loga todas as informações da câmera no formato de tabela.
 *
 * Delega para ImplCameraComponent_DMV_SDK::logCameraInfo().
 *
 * @param cam  Componente de câmera conectado.
 */
void logCameraInfo(const CCameraComponent& cam);

// ─────────────────────────────────────────────────────────────────────────────
// Feature Stream (salvar/carregar configurações completas em arquivo)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Salva todos os parâmetros GenICam em um arquivo .dfs.
 *
 * O arquivo Delta Feature Stream (.dfs) pode ser usado para backup ou
 * para restaurar configurações em outra câmera do mesmo modelo.
 *
 * @param cam       Componente de câmera conectado (não deve estar em stream).
 * @param filepath  Caminho do arquivo de saída (ex: "camera_config.dfs").
 * @return          CameraStatus::Ok em sucesso.
 */
CameraStatus saveFeatureStream(const CCameraComponent& cam,
                               const std::string& filepath);

/**
 * @brief Carrega parâmetros GenICam de um arquivo .dfs.
 *
 * Em caso de erros parciais, o SDK continua carregando o restante do arquivo
 * e retorna a mensagem de erro ao final.
 *
 * @param cam       Componente de câmera conectado (não deve estar em stream).
 * @param filepath  Caminho do arquivo .dfs a carregar.
 * @return          CameraStatus::Ok em sucesso; InternalError com log se falhar.
 */
CameraStatus loadFeatureStream(CCameraComponent& cam,
                               const std::string& filepath);

// ─────────────────────────────────────────────────────────────────────────────
// Utilitários
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Constrói uma CameraConfig com valores padrão do projeto Falcor.
 *
 * Atalho para criar a configuração de conexão sem precisar preencher
 * manualmente todos os campos do struct.
 *
 * @param ip      IP da câmera (ex: "192.168.1.100").
 * @param width   Largura em pixels (padrão: 1920).
 * @param height  Altura em pixels (padrão: 1080).
 * @param fps     Taxa de aquisição (padrão: 30.0).
 * @return        CameraConfig pronta para uso em CCameraComponent::initialize().
 */
CameraConfig makeConfig(const std::string& ip,
                        uint32_t width  = 1920,
                        uint32_t height = 1080,
                        double   fps    = 30.0);

} // namespace Falcor::Delta
