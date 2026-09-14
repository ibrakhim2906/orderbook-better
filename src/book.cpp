
#include "orderbook/book.hpp"

namespace orderbook {

void Book::apply(const Command& command, std::vector<Fill>& out) {
    (void)out;
    switch (command.type) {
    case CommandType::Add: break;
    case CommandType::Cancel: break;
    case CommandType::Execute: break;
    case CommandType::Reduce: break;
    case CommandType::Replace: break;
    }
}
}
