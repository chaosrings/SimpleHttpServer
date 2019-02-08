#include "Abstract.h"

class TextPlain :public Abstract{
private:
    std::string text;
public:
    TextPlain():Abstract(),text(""){};
    bool parser(std::string& body_buf) override;
};