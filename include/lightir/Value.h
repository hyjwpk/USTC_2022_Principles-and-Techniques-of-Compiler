#ifndef SYSYC_VALUE_H
#define SYSYC_VALUE_H

#include <functional>
#include <iostream>
#include <list>
#include <string>

class Type;
class Value;
class User;

struct Use {
    Value *val_;
    unsigned arg_no_; // the no. of operand, e.g., func(a, b), a is 0, b is 1
    Use(Value *val, unsigned no) : val_(val), arg_no_(no) {}
};

class Value {
  public:
    explicit Value(Type *ty, const std::string &name = "");
    virtual ~Value() = default;

    Type *get_type() const { return type_; }

    std::list<Use> &get_use_list() { return use_list_; }

    void add_use(Value *val, unsigned arg_no = 0);

    bool set_name(std::string name) {
        if (name_ == "") {
            name_ = name;
            return true;
        }
        return false;
    }
    std::string get_name() const;

    void replace_all_use_with(Value *new_val);
    /// replace `value` with `new_val` when the user of value satisfies predicate `pred`
    void replace_use_with_when(Value *new_val, std::function<bool(User *)> pred);
    void remove_use(Value *val);

    virtual std::string print() = 0;

  private:
    Type *type_;
    std::list<Use> use_list_; // who use this value
    std::string name_;        // should we put name field here ?
};

#endif // SYSYC_VALUE_H
