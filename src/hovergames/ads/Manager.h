#ifndef HOVERGAMES_ADS_MANAGER_H
#define HOVERGAMES_ADS_MANAGER_H

#include <vector>
#include <chrono>
#include <random>

#include <hovergames/ads/Provider.h>

namespace hovergames {
namespace ads {

class Manager
{
public:
    // Configuration settings
    static int cooldownInSeconds;
    static int onlyShowEveryNThAd;
    static int dontShowFirstAdOnAppStart;
    static bool enabled;

    static std::vector<Provider*> providers;
    static void configureFromFile(const std::string& filename);
    static void initialize();
    static bool isCooldownActive();

    // Proxy methods to a random provider
    static void showFullscreen(const bool ignoreCooldown = false);
    static void showBanner(const bool ignoreCooldown = false);
    static void showPopup(const bool ignoreCooldown = false);
    static void openLink(const bool ignoreCooldown = false);
    static void hideAll();

    template <typename T>
    static void callRandomProvider(const bool ignoreCooldown, std::function<void(T*)> callback)
    {
        auto provider = getRandomProvider<T>();
        if (!provider) {
            return;
        }

        if (!ignoreCooldown && isCooldownActive()) {
            return;
        }

        callback(provider);
        lastAdShownAt = std::chrono::seconds(std::time(NULL)).count();
    }

    template <typename T>
    static T* getRandomProvider()
    {
        // sum up all weights for matching providers
        int sumWeight = 0;
        for (auto& provider : providers) {
            auto providerT = dynamic_cast<T*>(provider);
            if (providerT) {
                sumWeight += provider->weight;
            }
        }

        // roll the dice
        static std::default_random_engine engine;
        static std::uniform_int_distribution<> dice{};
        using parm_t = decltype(dice)::param_type;
        auto randomNumber = dice(engine, parm_t{0, sumWeight});

        // go and select the lucky provider
        for (auto& provider : providers) {
            auto providerT = dynamic_cast<T*>(provider);
            if (providerT) {
                randomNumber -= provider->weight;
                if (randomNumber <= 0) {
                    return providerT;
                }
            }
        }

        return nullptr;
    }

private:
    // prevent instantiation
    Manager() = delete;
    ~Manager() = delete;
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;

    static bool bannerVisible;
    static int counter;
    static int lastAdShownAt;
};

} // namespace ads
} // namespace hovergames

#endif /* HOVERGAMES_ADS_MANAGER_H */
