#include <map>
#include <string>
#include <vector>

class SymbolList {
    std::vector<std::map<std::string, void *>> sym_stk;

public:
    void newEnv() {
        sym_stk.push_back({});
    }

    void addSymbol(const std::string & name, void * item) {
        sym_stk.rbegin()->insert_or_assign(name, item);
    }

    void * getSymbol(const std::string & name) {
        for (auto m = sym_stk.rbegin(); m != sym_stk.rend(); ++m)
            if (m->count(name))
                return m->operator[](name);
        return nullptr;
    }

    void deleteEnv() {
        sym_stk.pop_back();
    }
};