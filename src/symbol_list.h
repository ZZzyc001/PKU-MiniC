#include <map>
#include <string>
#include <vector>

class LValSymbol {
public:
    enum class SymbolType {
        Const,
        Var,
        Array,
        Pointer,
        Function
    } type;
    void * number;
    LValSymbol() = default;
    LValSymbol(SymbolType t, void * n):
        type(t), number(n) {}
};

class SymbolList {
    std::vector<std::map<std::string, LValSymbol>> sym_stk;

public:
    void newEnv() {
        sym_stk.push_back({});
    }

    void addSymbol(const std::string & name, LValSymbol item) {
        sym_stk.rbegin()->insert_or_assign(name, item);
    }

    LValSymbol getSymbol(const std::string & name) {
        for (auto m = sym_stk.rbegin(); m != sym_stk.rend(); ++m)
            if (m->count(name))
                return m->operator[](name);
        return LValSymbol();
    }

    void deleteEnv() {
        sym_stk.pop_back();
    }
};