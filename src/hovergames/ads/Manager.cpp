#include <hovergames/ads/Manager.h>

#include "cocos2d.h"
#include <hovergames/platform.h>
#include <cpptoml.h>

#include <hovergames/ads/Banner.h>
#include <hovergames/ads/Fullscreen.h>
#include <hovergames/ads/Link.h>
#include <hovergames/ads/Popup.h>

#if !HOVERGAMES_PLATFORM_IS_ANDROID_SAMSUNG
    #if defined(COCOWIZARD_HOVERGAMES_ADS_CHARTBOOST)
        #include <hovergames/ads/provider/Chartboost.h>
    #endif
    #if defined(COCOWIZARD_HOVERGAMES_ADS_REVMOB)
        #include <hovergames/ads/provider/Revmob.h>
    #endif
    #if defined(COCOWIZARD_HOVERGAMES_ADS_IAD) && HOVERGAMES_PLATFORM_IS_IOS
        #include <hovergames/ads/provider/IAd.h>
    #endif
    #if defined(COCOWIZARD_HOVERGAMES_ADS_VUNGLE) && HOVERGAMES_PLATFORM_IS_IOS
        #include <hovergames/ads/provider/Vungle.h>
    #endif
    #if defined(COCOWIZARD_HOVERGAMES_ADS_TAPFORTAP) && HOVERGAMES_PLATFORM_IS_IOS
        #include <hovergames/ads/provider/TapForTap.h>
    #endif
#endif

namespace hovergames {
namespace ads {

int Manager::cooldownInSeconds = 60;
int Manager::onlyShowEveryNThAd = 1;
int Manager::dontShowFirstAdOnAppStart = 0;
bool Manager::enabled = false;
std::vector<Provider*> Manager::providers;
int Manager::counter = 0;
int Manager::lastAdShownAt = 0;
bool Manager::bannerVisible = false;

void Manager::configureFromFile(const std::string& filename)
{
    auto file = cocos2d::FileUtils::getInstance()->fullPathForFilename(filename);
    auto config = cpptoml::parse_file(file);

    auto configInt = [&config](const std::string& key, const int fallback) {
        auto value = config.find_as<int64_t>(key);
        return value ? *value : fallback;
    };
    auto configString = [&config](const std::string& key) {
        auto value = config.find_as<std::string>(key);
        return value ? *value : "";
    };
    auto checkRegister = [](Provider* provider, std::function<bool()> cb) {
        if (provider->weight <= 0 || !cb()) {
            delete provider;
        } else {
            providers.push_back(provider);
        }
    };

    cooldownInSeconds = configInt("general.cooldownInMinutes", 1) * 60;
    onlyShowEveryNThAd = configInt("general.onlyShowEveryNThAd", 1);
    dontShowFirstAdOnAppStart = configInt("general.dontShowFirstAdOnAppStart", 0);
    counter = onlyShowEveryNThAd - 1; // ensure we show the next ad!
    enabled = true;

    auto flavor = hovergames::platform::getFlavor();
    flavor[0] = std::toupper(flavor[0]);
    auto prefix = hovergames::platform::getName() + flavor;

#if defined(COCOWIZARD_HOVERGAMES_ADS_CHARTBOOST) && !HOVERGAMES_PLATFORM_IS_ANDROID_SAMSUNG
    if (config.contains("chartboost")) {
        auto p = new provider::Chartboost();
        p->weight = configInt("chartboost.weight", 1);
        p->appId = configString("chartboost." + prefix + "AppId");
        p->appSignature = configString("chartboost." + prefix + "AppSignature");

        checkRegister(p, [&p]() {
            return !p->appId.empty() && !p->appSignature.empty();
        });
    }
#endif

#if defined(COCOWIZARD_HOVERGAMES_ADS_REVMOB) && !HOVERGAMES_PLATFORM_IS_ANDROID_SAMSUNG
    if (config.contains("revmob")) {
        auto p = new provider::Revmob();
        p->weight = configInt("revmob.weight", 1);
        p->appId = configString("revmob." + prefix + "AppId");

        checkRegister(p, [&p]() { return !p->appId.empty(); });
    }
#endif

#if defined(COCOWIZARD_HOVERGAMES_ADS_VUNGLE) && HOVERGAMES_PLATFORM_IS_IOS
    if (config.contains("vungle")) {
        auto p = new provider::Vungle();
        p->weight = configInt("vungle.weight", 1);
        p->appId = configString("vungle." + prefix + "AppId");

        checkRegister(p, [&p]() { return !p->appId.empty(); });
    }
#endif

#if defined(COCOWIZARD_HOVERGAMES_ADS_TAPFORTAP) && HOVERGAMES_PLATFORM_IS_IOS
    if (config.contains("tapfortap")) {
        auto p = new provider::TapForTap();
        p->weight = configInt("tapfortap.weight", 1);
        p->apiKey = configString("tapfortap.apiKey");

        checkRegister(p, [&p]() { return !p->apiKey.empty(); });
    }
#endif

#if defined(COCOWIZARD_HOVERGAMES_ADS_IAD) && HOVERGAMES_PLATFORM_IS_IOS
    if (config.contains("iad")) {
        auto p = new provider::IAd();
        p->weight = configInt("iad.weight", 1);

        checkRegister(p, [&p]() { return true; });
    }
#endif
}

void Manager::initialize()
{
    for (auto& provider : providers) {
        provider->initialize();
    }
}

void Manager::showFullscreen(const bool ignoreCooldown)
{
    callRandomProvider<Fullscreen>(ignoreCooldown, [](Fullscreen* provider) {
        provider->showFullscreen();
    });
}

void Manager::showBanner(const bool ignoreCooldown)
{
    if (bannerVisible) {
        return;
    }

    callRandomProvider<Banner>(ignoreCooldown, [](Banner* provider) {
        provider->showBanner();
        bannerVisible = true;
    });
}

void Manager::showPopup(const bool ignoreCooldown)
{
    callRandomProvider<Popup>(ignoreCooldown, [](Popup* provider) {
        provider->showPopup();
    });
}

void Manager::openLink(const bool ignoreCooldown)
{
    callRandomProvider<Link>(ignoreCooldown, [](Link* provider) {
        provider->openLink();
    });
}

void Manager::hideAll()
{
    for (auto& provider : providers) {
        provider->hideAll();
    }
    bannerVisible = false;
}

bool Manager::isCooldownActive()
{
    if (!enabled) {
        return true;
    }

    if (dontShowFirstAdOnAppStart > 0) {
        --dontShowFirstAdOnAppStart;
        return true;
    }

    ++counter;
    if (onlyShowEveryNThAd > 0 && (counter % onlyShowEveryNThAd) != 0) {
        return true;
    }

    auto now = std::chrono::seconds(std::time(NULL)).count();
    if ((lastAdShownAt + cooldownInSeconds) > now) {
        return true;
    }

    return false;
}

} // namespace ads
} // namespace hovergames
