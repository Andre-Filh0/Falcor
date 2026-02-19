#include <QCoreApplication>
#include <QTimer>
#include <iostream>
#include "Network/RFCOMMSockets.hpp"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    std::cout << "========================================" << std::endl;
    std::cout << "   FALCOR CORE - KRIA KV260 EDITION" << std::endl;
    std::cout << "========================================" << std::endl;

    RFCOMMSockets<std::string> rfcomm;

    QTimer mainLoopTimer;
    QObject::connect(&mainLoopTimer, &QTimer::timeout, [&]() {
        if (!rfcomm.is_ready()) {
            std::cout << "[SYSTEM] Buscando dispositivo..." << std::endl;
            rfcomm.listen();
        } else {
            rfcomm.Heartbeat();
            std::cout << "[SYSTEM] Heartbeat OK - Link Ativo" << std::endl;
        }
    });

    mainLoopTimer.start(5000);
    return app.exec();
}