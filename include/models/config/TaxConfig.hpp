#pragma once

class TaxConfig {
private:
    int pphFlat;
    int pphPercentage;
    int pbmFlat;

public:
    TaxConfig();
    TaxConfig(int pphFlat, int pphPercentage, int pbmFlat);

    int getPphFlat() const;
    int getPphPercentage() const;
    int getPbmFlat() const;
};
