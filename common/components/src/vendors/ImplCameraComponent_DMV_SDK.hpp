#pragma once

#include "Logger.hpp"
#include "System.hpp"
#include "../interfaces/ICameraComponent.hpp"
#include "dmvc/dmvc.h"

#include <atomic>
#include <vector>
#include <string>
#include <optional>

// ── Constantes do SDK ─────────────────────────────────────────────────────────
#define DMV_BUFFER_COUNT        16
#define DMV_IMAGE_WAIT_MS     2000
#define DMV_ENUM_TIMEOUT_MS   2000
#define DMV_NODE_BUF_SIZE      256

// ─────────────────────────────────────────────────────────────────────────────
// Tipos Delta-específicos
// Definidos aqui para que src/Camera.hpp possa reutilizá-los sem duplicação.
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief User Presets GenICam (presentes em todas as câmeras Delta GigE Vision).
 *
 * Default   = fábrica (somente leitura)
 * UserSet1–4 = presets configuráveis pelo usuário
 *
 * Uso típico: carrega UserSet1 na inicialização, que contém
 * a resolução, exposure e gain ideais para o ambiente do curral.
 */
enum class DeltaUserPreset {
    Default  = 0,
    UserSet1 = 1,
    UserSet2 = 2,
    UserSet3 = 3,
    UserSet4 = 4,
};

/**
 * @brief Informações completas da câmera lidas diretamente dos nós GenICam.
 *
 * Preenchido por ImplCameraComponent_DMV_SDK::queryCameraInfo().
 * Todos os campos têm valor padrão caso o nó não esteja disponível.
 */
struct DeltaCameraInfo {
    // ── Identificação ──────────────────────────────────────────────────────
    std::string model         = "N/A";  // ex: "MV-CU120-90GC"
    std::string serial        = "N/A";  // número de série único
    std::string userID        = "N/A";  // ID definido pelo usuário (se configurado)
    std::string manufacturer  = "N/A";  // "Delta Electronics"
    std::string firmware      = "N/A";  // versão do firmware

    // ── Rede GigE ──────────────────────────────────────────────────────────
    std::string ip            = "N/A";  // IP atual da câmera
    std::string mac           = "N/A";  // endereço MAC
    std::string subnet        = "N/A";  // máscara de sub-rede

    // ── Configuração de imagem ─────────────────────────────────────────────
    std::string pixelFormat   = "N/A";  // ex: "RGB8Packed", "BayerRG8"
    uint32_t    width         = 0;
    uint32_t    height        = 0;
    double      fps           = 0.0;    // taxa de aquisição configurada
    double      exposureUs    = 0.0;    // exposição em microssegundos
    double      gainDb        = 0.0;    // ganho analógico em dB

    // ── User Presets ───────────────────────────────────────────────────────
    DeltaUserPreset activePreset = DeltaUserPreset::Default;
    std::string     presetName   = "Default";
};

/**
 * @brief Câmera encontrada durante varredura de rede (antes de conectar).
 *
 * Retornado por ImplCameraComponent_DMV_SDK::enumerateCameras().
 */
struct DeltaDiscoveredCamera {
    std::string ip;
    std::string mac;
    std::string model;
    std::string serial;
};

// ─────────────────────────────────────────────────────────────────────────────
// Implementação concreta para câmeras Delta GigE Vision (DMV SDK)
// ─────────────────────────────────────────────────────────────────────────────

class ImplCameraComponent_DMV_SDK final : public ICameraComponent
{
public:
    explicit ImplCameraComponent_DMV_SDK(const CameraConfig& config);
    ~ImplCameraComponent_DMV_SDK() override;

    // ── ICameraComponent (interface obrigatória) ──────────────────────────
    CameraStatus connect   (const CameraConfig& config) override;
    void         disconnect()                           override;
    bool         isConnected() const                   override;

    CameraStatus startStream() override;
    void         stopStream()  override;
    bool         isStreaming() const override;

    CameraStatus getRawFrame(RawFrame& outFrame) override;

    uint32_t    getWidth()       const override;
    uint32_t    getHeight()      const override;
    PixelFormat getPixelFormat() const override;
    const char* getCameraName()  const override;

    // ── API específica Delta (chamada via src/Camera.hpp) ─────────────────

    /**
     * @brief Carrega um User Preset da memória da câmera.
     *
     * Equivale a selecionar o preset no menu da câmera e clicar "Load".
     * Os parâmetros (exposure, gain, resolução, etc.) são restaurados
     * imediatamente. Requer câmera conectada mas não necessariamente em stream.
     *
     * @return CameraStatus::Ok em sucesso, InternalError se o nó não aceitar.
     */
    CameraStatus loadUserPreset(DeltaUserPreset preset);

    /**
     * @brief Salva as configurações atuais em um User Preset.
     *
     * Grava os parâmetros atuais (exposure, gain, resolução, formato,
     * trigger, etc.) no slot indicado. Default é somente leitura e
     * será rejeitado pelo SDK com InternalError.
     *
     * @return CameraStatus::Ok em sucesso.
     */
    CameraStatus saveUserPreset(DeltaUserPreset preset);

    /**
     * @brief Define qual User Preset será carregado automaticamente ao ligar.
     *
     * @return CameraStatus::Ok em sucesso.
     */
    CameraStatus setDefaultPreset(DeltaUserPreset preset);

    /**
     * @brief Lê todos os nós relevantes e retorna um DeltaCameraInfo populado.
     *
     * Requer câmera conectada (m_nodelist válido).
     * Campos não disponíveis ficam com valor "N/A" ou 0.
     */
    DeltaCameraInfo queryCameraInfo() const;

    /**
     * @brief Loga todas as informações da câmera via Falcor::Logger.
     *
     * Formato destacado: modelo, serial, IP, MAC, firmware,
     * resolução, FPS, exposure, gain e preset ativo.
     */
    void logCameraInfo() const;

    // ── Varredura de rede (estática, sem precisar de IP) ─────────────────

    /**
     * @brief Descobre todas as câmeras Delta presentes na rede.
     *
     * Fluxo: DcSystem → UpdateInterfaceList → Interface → UpdateDeviceList
     *        → DcDeviceGetInfo + DeviceSelector → IP/MAC da interface nodelist.
     * Não é necessário saber o IP previamente — ideal para auto-descoberta.
     *
     * @param timeoutMs  Tempo máximo de varredura por interface (padrão: 2 s).
     * @return           Vetor de câmeras encontradas (pode ser vazio).
     */
    static std::vector<DeltaDiscoveredCamera> enumerateCameras(
        uint32_t timeoutMs = DMV_ENUM_TIMEOUT_MS);

    /**
     * @brief Salva todos os parâmetros GenICam em um arquivo .dfs.
     *
     * Delega para DcNodeListSaveFeatureStream. Câmera deve estar conectada
     * e fora de estado de aquisição.
     *
     * @param filepath  Caminho do arquivo de saída (ex: "camera_config.dfs").
     * @return          CameraStatus::Ok em sucesso.
     */
    CameraStatus saveFeatureStreamToFile(const std::string& filepath) const;

    /**
     * @brief Carrega parâmetros GenICam de um arquivo .dfs.
     *
     * Em caso de erros parciais, o SDK continua até o fim do arquivo e
     * retorna a mensagem de erro acumulada.
     *
     * @param filepath  Caminho do arquivo .dfs a carregar.
     * @return          CameraStatus::Ok em sucesso; InternalError com log.
     */
    CameraStatus loadFeatureStreamFromFile(const std::string& filepath);

private:
    void resetHandles() noexcept;

    // ── Helpers de nó GenICam ─────────────────────────────────────────────
    // Nível 1 (string): DcNodeListGetValue / DcNodeListSetValue
    std::string readNodeStr  (const char* nodeName) const;
    // Nível 2 (tipado): GetNode → CastToFloatNode → FloatNodeGetValue
    double      readNodeFloat(const char* nodeName) const;
    // Nível 2 (tipado): GetNode → CastToIntegerNode → IntegerNodeGetValue
    int64_t     readNodeInt  (const char* nodeName) const;

    // Executa nó de comando: GetNode → CastToCommandNode → CommandNodeExecute
    // NOTA: DcNodeListExecute() NÃO EXISTE no SDK.
    CameraStatus executeNode(const char* nodeName);

    // Converte enum DeltaUserPreset para a string GenICam correspondente.
    static const char* presetToStr(DeltaUserPreset preset);

private:
    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_streaming{false};

    CameraConfig m_config;
    uint32_t     m_frameCounter{0};

    DcSystem     m_system{nullptr};
    DcDevice     m_device{nullptr};
    DcDataStream m_stream{nullptr};
    DcNodeList   m_nodelist{nullptr};

    std::vector<DcBuffer> m_buffers;
};
