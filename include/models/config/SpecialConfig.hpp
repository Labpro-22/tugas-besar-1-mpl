#pragma once

class SpecialConfig {
private:
    int goSalary;
    int jailFine;

public:
    SpecialConfig();
    SpecialConfig(int goSalary, int jailFine);

    int getGoSalary() const;
    int getJailFine() const;
};
