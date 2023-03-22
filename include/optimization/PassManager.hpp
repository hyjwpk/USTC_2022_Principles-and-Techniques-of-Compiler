#ifndef PASSMANAGER_HPP
#define PASSMANAGER_HPP

#include "Module.h"

#include <memory>
#include <vector>

class Pass {
  public:
    Pass(Module *m) : m_(m) {}
    virtual ~Pass() = default;
    virtual void run() = 0;

  protected:
    Module *m_;
};

class PassManager {
  public:
    PassManager(Module *m) : m_(m) {}
    template <typename PassType, typename... Args>
    void add_pass(bool print_ir, Args &&...args) {
        passes_.push_back(
            std::pair<std::unique_ptr<Pass>, bool>(new PassType(m_, std::forward<Args>(args)...), print_ir));
    }
    void run() {
        for (auto &pass : passes_) {
            pass.first->run();
            if (pass.second) {
                m_->set_print_name();
                std::cout << m_->print();
            }
        }
    }

  private:
    std::vector<std::pair<std::unique_ptr<Pass>, bool>> passes_;
    Module *m_;
};

#endif