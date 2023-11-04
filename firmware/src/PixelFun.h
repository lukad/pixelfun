#pragma once

#include <cstdlib>
#include <stack>
#include <cctype>
#include <cstring>

enum ExprType {
    EXPR_NUMBER,
    EXPR_BINOP,
    EXPR_VAR,
    EXPR_FUNC,
};

enum BinOpType {
    BINOP_ADD,
    BINOP_SUB,
    BINOP_MUL,
    BINOP_DIV,
};

enum Var {
    VAR_T,
    VAR_I,
    VAR_X,
    VAR_Y,
    VAR_PI,
    VAR_TAU,
};

enum FuncType {
    FUNC_SIN,
    FUNC_COS,
    FUNC_TAN,
    FUNC_HYPOT,
};

struct Expr {
    ExprType type;
    union {
        float number;
        Var var;
        struct {
            BinOpType op;
            Expr *a;
            Expr *b;
        } binop;
        struct {
            FuncType func;
            Expr *args[2];
            size_t arity;
        } funcCall;
    };
} typedef Expr;

template<size_t desired_capacity>
class PixelFun {
private:
    Expr pool[desired_capacity];
    size_t freeIndices[desired_capacity];
    size_t stackTop;
    Expr *root;

public:
    PixelFun() : pool(), stackTop(desired_capacity), freeIndices(), root(nullptr) {
        for (size_t i = 0; i < desired_capacity; i++) {
            freeIndices[i] = i;
        }
    }

    bool parse(const char *expr) {
        const char *rest = parseExpr(expr, root);
        return rest && *rest == '\0';
    }

    float eval(float t, float i, float x, float y) {
        return eval(root, t, i, x, y);
    }

    void printAST() {
        printAST(root, 0);
    }

private:
    Expr *alloc(ExprType exprType) {
        if (stackTop == 0) {
            return nullptr;
        }

        size_t idx = freeIndices[--stackTop];
        pool[idx].type = exprType;
        return &pool[idx];
    }

//    void dealloc(Expr *expr) {
//        size_t index = expr - pool;
//        freeIndices[stackTop++] = index;
//    }

    float eval(Expr *expr, float t, float i, float x, float y) {
        if (!expr) {
            return 0;
        }

        switch (expr->type) {
            case EXPR_NUMBER:
                return expr->number;
            case EXPR_VAR:
                switch (expr->var) {
                    case VAR_T:
                        return t;
                    case VAR_I:
                        return i;
                    case VAR_X:
                        return x;
                    case VAR_Y:
                        return y;
                    case VAR_PI:
                        return PI;
                    case VAR_TAU:
                        return 2 * PI;
                }
            case EXPR_FUNC:
                switch (expr->funcCall.func) {
                    case FUNC_SIN:
                        return sinf(eval(expr->funcCall.args[0], t, i, x, y));
                    case FUNC_COS:
                        return cosf(eval(expr->funcCall.args[0], t, i, x, y));
                    case FUNC_TAN:
                        return tanf(eval(expr->funcCall.args[0], t, i, x, y));
                    case FUNC_HYPOT:
                        return sqrt(pow(eval(expr->funcCall.args[0], t, i, x, y), 2.0f) +
                                    pow(eval(expr->funcCall.args[1], t, i, x, y), 2.0f));
                }
            case EXPR_BINOP:
                float lhs = eval(expr->binop.a, t, i, x, y);
                float rhs = eval(expr->binop.b, t, i, x, y);
                switch (expr->binop.op) {
                    case BINOP_ADD:
                        return lhs + rhs;
                    case BINOP_SUB:
                        return lhs - rhs;
                    case BINOP_MUL:
                        return lhs * rhs;
                    case BINOP_DIV:
                        if (rhs == 0) {
                            return 0;
                        }
                        return lhs / rhs;
                }
        }

        return 0;
    }

    const char *parseExpr(const char *input, Expr *&node) {
        input = parseTerm(input, node);
        if (!input) {
            return nullptr;
        }

        while (true) {
            while (*input && isspace(*input)) {
                input++;
            }

            BinOpType op;
            const auto nextInput = parseBinOp(input, op);
            if (!nextInput || (op != BINOP_ADD && op != BINOP_SUB)) {
                break;
            }

            input = nextInput;
            while (*input && isspace(*input)) {
                input++;
            }

            Expr *rhs = nullptr;
            input = parseTerm(input, rhs);
            if (!input) {
                return nullptr;
            }

            Expr *binOp = alloc(EXPR_BINOP);
            if (!binOp) {
                return nullptr;
            }

            binOp->binop.op = op;
            binOp->binop.a = node;
            binOp->binop.b = rhs;
            node = binOp;
        }

        return input;
    }

    const char *parseNumber(const char *exprStr, Expr *&expr) {
        char *end;
        float value = strtof(exprStr, &end);
        if (end == exprStr) {
            return nullptr;
        }

        expr = alloc(EXPR_NUMBER);
        if (!expr) {
            return nullptr;
        }

        expr->number = value;
        return end;
    };

    const char *parseBinOp(const char *exprStr, BinOpType &op) {
        switch (*exprStr) {
            case '+':
                op = BINOP_ADD;
                break;
            case '-':
                op = BINOP_SUB;
                break;
            case '*':
                op = BINOP_MUL;
                break;
            case '/':
                op = BINOP_DIV;
                break;
            default:
                return nullptr;
        }
        return exprStr + 1;
    }

    const char *parseFactor(const char *input, Expr *&node) {
        if (*input == '(') {
            input = parseExpr(input + 1, node);
            if (!input || *input != ')') {
                return nullptr;
            }
            return input + 1;
        } else if (isAlpha(*input)) {
            const char *rest = parseFunction(input, node);
            if (rest) {
                return rest;
            }
            return parseIdentifier(input, node);
        }
        return parseNumber(input, node);
    }

    const char *parseTerm(const char *input, Expr *&node) {
        input = parseFactor(input, node);
        if (!input) {
            return nullptr;
        }

        while (true) {
            while (*input && isspace(*input)) {
                input++;
            }

            BinOpType op;
            const char *nextInput = parseBinOp(input, op);
            if (!nextInput || (op != BINOP_MUL && op != BINOP_DIV)) {
                break;
            }

            input = nextInput;
            while (*input && isspace(*input)) {
                input++;
            }

            Expr *rhs = nullptr;
            input = parseFactor(input, rhs);
            if (!input) {
                return nullptr;
            }

            Expr *binOp = alloc(EXPR_BINOP);
            if (!binOp) {
                return nullptr;
            }

            binOp->binop.op = op;
            binOp->binop.a = node;
            binOp->binop.b = rhs;
            node = binOp;
        }
        return input;
    }

    const char *parseIdentifier(const char *input, Expr *&node) {
        static const struct {
            const char *name;
            Var var;
        } vars[] = {
                {"t", VAR_T},
                {"i", VAR_I},
                {"x", VAR_X},
                {"y", VAR_Y},
        };

        for (size_t i = 0; i < sizeof(vars) / sizeof(vars[0]); i++) {
            size_t len = strlen(vars[i].name);
            if (strncmp(input, vars[i].name, len) == 0) {
                node = alloc(EXPR_VAR);
                if (!node) {
                    return nullptr;
                }
                node->var = vars[i].var;
                return input + len;
            }
        }

        return nullptr;
    }

    const char *parseFunction(const char *input, Expr *&node) {
        static const struct {
            const char *name;
            FuncType func;
            size_t arity;
        } funcs[]{
                {"sin",   FUNC_SIN,   1},
                {"cos",   FUNC_COS,   1},
                {"tan",   FUNC_TAN,   1},
                {"hypot", FUNC_HYPOT, 2},
        };

        for (size_t i = 0; i < sizeof(funcs) / sizeof(funcs[0]); i++) {
            size_t len = strlen(funcs[i].name);
            if (strncmp(input, funcs[i].name, len) == 0 && input[len] == '(') {
                node = alloc(EXPR_FUNC);
                if (!node) {
                    return nullptr;
                }
                node->funcCall.func = funcs[i].func;
                node->funcCall.arity = funcs[i].arity;
                input += len + 1;

                for (size_t arg = 0; arg < funcs[i].arity; arg++) {

                    if (arg > 0) {
                        if (*input != ',') return nullptr;  // Error: missing comma between arguments
                        input++;  // Skip comma
                    }
                    const char *nextInput = parseExpr(input, node->funcCall.args[arg]);
                    if (!nextInput) { return nullptr; }

                    if (arg == funcs[i].arity - 1 && *nextInput != ')') { return nullptr; }
                    if (arg < funcs[i].arity - 1 && *nextInput != ',') { return nullptr; }

                    input = nextInput;
                }
                return input + 1;
            }
        }

        return nullptr;
    }


    void printAST(Expr *node, int indent = 0) {
        if (!node) return;

        // Indent the current line to show the tree structure
        for (int i = 0; i < indent; ++i) {
            Serial.print("  ");
        }

        switch (node->type) {
            case EXPR_NUMBER:
                Serial.print("Float: ");
                Serial.println(node->number, 6);
                break;

            case EXPR_BINOP:
                Serial.print("BinOp: ");
                switch (node->binop.op) {
                    case BINOP_ADD:
                        Serial.println("ADD");
                        break;
                    case BINOP_SUB:
                        Serial.println("SUB");
                        break;
                    case BINOP_MUL:
                        Serial.println("MUL");
                        break;
                    case BINOP_DIV:
                        Serial.println("DIV");
                        break;
                }
                printAST(node->binop.a, indent + 1);
                printAST(node->binop.b, indent + 1);
                break;

            case EXPR_VAR:
                Serial.print("Var: ");
                switch (node->var) {
                    case VAR_T:
                        Serial.println("T");
                        break;
                    case VAR_I:
                        Serial.println("I");
                        break;
                    case VAR_X:
                        Serial.println("X");
                        break;
                    case VAR_Y:
                        Serial.println("Y");
                        break;
                    case VAR_PI:
                        Serial.println("PI");
                        break;
                    case VAR_TAU:
                        Serial.println("TAU");
                        break;
                }
                break;

            case EXPR_FUNC:
                Serial.print("Func: ");
                switch (node->funcCall.func) {
                    case FUNC_SIN:
                        Serial.println("SIN");
                        break;
                    case FUNC_COS:
                        Serial.println("COS");
                        break;
                    case FUNC_TAN:
                        Serial.println("TAN");
                        break;
                    case FUNC_HYPOT:
                        Serial.println("HYPOT");
                        break;
                }
                for (size_t i = 0; i < node->funcCall.arity; ++i) {
                    printAST(node->funcCall.args[i], indent + 1);
                }
                break;
        }
    }
};