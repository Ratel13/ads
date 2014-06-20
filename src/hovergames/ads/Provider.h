#ifndef HOVERGAMES_ADS_PROVIDER_H
#define HOVERGAMES_ADS_PROVIDER_H

namespace hovergames {
namespace ads {

class Provider
{
public:
    int weight = 1;

    virtual ~Provider() {}
    virtual void initialize() = 0;
    virtual void hideAll() = 0;
};

} // namespace ads
} // namespace hovergames

#endif /* HOVERGAMES_ADS_PROVIDER_H */
