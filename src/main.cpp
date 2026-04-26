#include <exception>

#include "app/CliApplication.hpp"
#include "utils/exceptions/ExceptionHandler.hpp"

int main()
{
    try {
        CliApplication app;
        return app.run();
    } catch (const std::exception& e) {
        ExceptionHandler::handle(e);
        return 1;
    } catch (...) {
        ExceptionHandler::handleUnknown();
        return 1;
    }
}
