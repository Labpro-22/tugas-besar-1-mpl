#pragma once

class Dice{
    private:
        int die1;
        int die2;

    public:
        Dice();
        void roll();
        void setManual(int d1, int d2);
        int getDie1() const;
        int getDie2() const;
        int getTotal() const;
        bool isDouble() const;
};