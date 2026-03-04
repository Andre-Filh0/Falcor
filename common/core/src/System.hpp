namespace Falcor::System {
    namespace CLI {
        // Mudamos para 'bool' para o resto do programa decidir se para ou continua
        inline bool executeCLICommand(const char *command) {
            Falcor::SysLog.log_debug("Executing system command: '{}'", command);
            
            // Executa o comando
            int raw_result = std::system(command);

            // Verifica se o comando foi executado e terminou normalmente
            // WIFEXITED e WEXITSTATUS são macros do <sys/wait.h> (padrão no Linux da Kria)
            int exit_code = WEXITSTATUS(raw_result);

            if (raw_result != -1 && exit_code == 0) {
                Falcor::SysLog.log_info("Command executed successfully (exit code 0)");
                return true;
            } else {
                Falcor::SysLog.log_error("Command failed or aborted. Exit code: {}", exit_code);
                return false;
            }
        }
    }
}