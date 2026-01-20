#pragma once

#include <memory>

#include "nodes/node.hpp"

class Parser;

class Statement : public Node {
    virtual auto parse(Parser& parser) -> std::unique_ptr<Statement> = 0;
};
