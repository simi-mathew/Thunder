/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "WarningReportingControl.h"
#include "JSON.h"
#include "Singleton.h"
#include "Sync.h"
#include "Thread.h"

#ifdef __CORE_WARNING_REPORTING__

namespace {
WPEFramework::Core::CriticalSection adminlock; // we cannot have this as a member as Sync.h might also need WarningReporting. but as WarningReportingUnitProxy that is not a problem

class WarningReportingBoundsCategoryConfig : public WPEFramework::Core::JSON::Container {
public:
    WarningReportingBoundsCategoryConfig(const WarningReportingBoundsCategoryConfig&) = delete;
    WarningReportingBoundsCategoryConfig& operator=(const WarningReportingBoundsCategoryConfig&) = delete;

    WarningReportingBoundsCategoryConfig()
        : WPEFramework::Core::JSON::Container()
        , ReportBound()
        , WarningBound()
        , CategoryConfig(false)
    {
        Add(_T("reportbound"), &ReportBound);
        Add(_T("warningbound"), &WarningBound);
        Add(_T("config"), &CategoryConfig);
    }

    ~WarningReportingBoundsCategoryConfig() override = default;

public:
    WPEFramework::Core::JSON::DecUInt64 ReportBound;
    WPEFramework::Core::JSON::DecUInt64 WarningBound;
    WPEFramework::Core::JSON::String CategoryConfig;
};

class ExcludedCategoriesValuesConfig : public WPEFramework::Core::JSON::Container {
public:
    ExcludedCategoriesValuesConfig(const WarningReportingBoundsCategoryConfig&) = delete;
    ExcludedCategoriesValuesConfig& operator=(const WarningReportingBoundsCategoryConfig&) = delete;

    ExcludedCategoriesValuesConfig()
        : WPEFramework::Core::JSON::Container()
        , ExcludedCallsigns()
        , ExcludedModules()
    {
        Add(_T("callsigns"), &ExcludedCallsigns);
        Add(_T("modules"), &ExcludedModules);
    }

    ~ExcludedCategoriesValuesConfig() override = default;

public:
    WPEFramework::Core::JSON::ArrayType<WPEFramework::Core::JSON::String> ExcludedCallsigns;
    WPEFramework::Core::JSON::ArrayType<WPEFramework::Core::JSON::String> ExcludedModules;
};
}

namespace WPEFramework {
namespace WarningReporting {

    WarningReportingUnitProxy& WarningReportingUnitProxy::Instance()
    {
        return (Core::SingletonType<WarningReportingUnitProxy>::Instance());
    }

    void WarningReportingUnitProxy::ReportWarningEvent(const char module[], const IWarningEvent& information)
    {
        adminlock.Lock();
        ASSERT (_handler != nullptr);
        if (_handler != nullptr) {
            _handler->ReportWarningEvent(module, information);
        }
        adminlock.Unlock();
    }

    void WarningReportingUnitProxy::FetchCategoryInformation(const string& category, bool& outIsDefaultCategory, bool& outIsEnabled, string& outExcluded, string& outConfiguration) const
    {
        adminlock.Lock();
        if (_handler != nullptr) {
            _handler->FetchCategoryInformation(category, outIsDefaultCategory, outIsEnabled, outExcluded, outConfiguration);
        }
        adminlock.Unlock();
    }

    void WarningReportingUnitProxy::AddToCategoryList(IWarningReportingUnit::IWarningReportingControl& Category)
    {
        adminlock.Lock();
        if (_handler != nullptr) {
            _handler->AddToCategoryList(Category);
        } else {
            _waitingAnnounces.emplace_back(&Category);
        }
        adminlock.Unlock();

    }

    void WarningReportingUnitProxy::RemoveFromCategoryList(IWarningReportingUnit::IWarningReportingControl& Category)
    {
        adminlock.Lock();
        if (_handler != nullptr) {
            ASSERT(_waitingAnnounces.size() == 0);
            _handler->RemoveFromCategoryList(Category);
        } else {
            WaitingAnnounceContainer::iterator it = std::find(std::begin(_waitingAnnounces), std::end(_waitingAnnounces), &Category);
            if (it != std::end(_waitingAnnounces)) {
                _waitingAnnounces.erase(it);
            }
        }
        adminlock.Unlock();
    }

    void WarningReportingUnitProxy::Handle(IWarningReportingUnit* handler)
    {
        ASSERT((_handler == nullptr && handler != nullptr) || (_handler != nullptr && handler == nullptr));
        adminlock.Lock();
        _handler = handler;
        if (_handler != nullptr) {

            for (IWarningReportingUnit::IWarningReportingControl* category : _waitingAnnounces) {
                ASSERT(category != nullptr);
                _handler->AddToCategoryList(*category);
            }
            _waitingAnnounces.clear();
        }
        adminlock.Unlock();
    }

    void WarningReportingUnitProxy::FillBoundsConfig(const string& boundsConfig, uint32_t& outReportingBound, uint32_t& outWarningBound, string& outSpecificConfig) const
    {
        WarningReportingBoundsCategoryConfig boundsconfig;

        boundsconfig.FromString(boundsConfig);

        if (boundsconfig.ReportBound.IsSet()) {
            outReportingBound = static_cast<uint32_t>(boundsconfig.ReportBound.Value());
        }

        if (boundsconfig.WarningBound.IsSet()) {
            outWarningBound = static_cast<uint32_t>(boundsconfig.WarningBound.Value());
        }

        if (boundsconfig.CategoryConfig.IsSet()) {
            outSpecificConfig = boundsconfig.CategoryConfig.Value();
        }

        if (outReportingBound > outWarningBound) {
            TRACE_L1("WarningReporting report bound [%d] is greater than waning bound [%d]!", outReportingBound, outWarningBound);
        }
        ASSERT(outReportingBound <= outWarningBound);
    }

    void WarningReportingUnitProxy::FillExcludedWarnings(const string& excludedJsonList, ExcludedWarnings& outExcludedWarnings) const
    {
        ExcludedCategoriesValuesConfig config;
        config.FromString(excludedJsonList);

        if (config.ExcludedCallsigns.IsSet()) {
            auto iterator(config.ExcludedCallsigns.Elements());
            while (iterator.Next()) {
                outExcludedWarnings.InsertCallsign(iterator.Current().Value());
            }
        }
        if (config.ExcludedModules.IsSet()) {
            auto iterator(config.ExcludedModules.Elements());
            while (iterator.Next()) {
                outExcludedWarnings.InsertModule(iterator.Current().Value());
            }
        }
    }
}
}

#endif
