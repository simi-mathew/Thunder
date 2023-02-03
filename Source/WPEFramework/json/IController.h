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

#pragma once
#include "Module.h"

// @stubgen:include <com/IIteratorType.h>
// @stubgen:include <plugins/MetaData.h>

namespace WPEFramework {
namespace Exchange {

    /* @json */
    struct EXTERNAL IController : virtual public Core::IUnknown {
        enum { ID = RPC::ID_CONTROLLER };

        struct CloneParams {
            string callsign;
            string newcallsign;
	};

        struct Callstack {
//            Core::instance_id address;
            string function;
            string module;
            uint32_t line;
	};

        struct SubsytemsParam {
            PluginHost::ISubSystem::subsystem subsystem;
            bool active;
        };

        using ICallstackDataIterator = RPC::IIteratorType<Callstack, RPC::ID_CONTROLLER_CALLSTACK_DATA_ITERATOR>;
        using ISubsystemsParamsDataIterator = RPC::IIteratorType<SubsytemsParam, RPC::ID_CONTROLLER_SUBSYSTEMS_PARAMS_DATA_ITERATOR>;

        /*using IMetaDataServiceIterator = RPC::IIteratorType<PluginHost::MetaData::Service, RPC::ID_CONTROLLER_META_DATA_SERVICE_ITERATOR>;
        using IMetaDataChannelIterator = RPC::IIteratorType<PluginHost::MetaData::Channel, RPC::ID_CONTROLLER_META_DATA_CHANNEL_ITERATOR>;
        using IMetaDataBridgeIterator = RPC::IIteratorType<PluginHost::MetaData::Bridge, RPC::ID_CONTROLLER_META_DATA_BRIDGE_ITERATOR>;*/
        // @event
        struct EXTERNAL INotification : virtual public Core::IUnknown {
            enum { ID = ID_CONTROLLER_NOTIFICATION };
            ~INotification() override = default;
        };

        ~IController() override = default;

        // Pushing notifications to interested sinks
        virtual uint32_t Register(IController::INotification* sink) = 0;
        virtual uint32_t Unregister(IController::INotification* sink) = 0;

	virtual uint32_t Activate(const string& callsign) = 0;
        virtual uint32_t Deactivate(const string& callsign) = 0;
        virtual uint32_t Unavailable(const string& callsign) = 0;
        virtual uint32_t Suspend(const string& callsign) = 0;
        virtual uint32_t Resume(const string& callsign) = 0;
        virtual uint32_t Clone(const CloneParams cloneparams, string& response /* @out */) = 0;
        virtual uint32_t Harakiri() = 0;
        virtual uint32_t StartDiscovery(const uint8_t& ttl) = 0;
        virtual uint32_t StoreConfig() = 0;
        virtual uint32_t Delete(const string& path) = 0;

	// @property
        virtual uint32_t CallStack(const string& index /* @index */, ICallstackDataIterator*& callstack /* @out */) const = 0;
	// @property
        virtual uint32_t Subsystems(ISubsystemsParamsDataIterator*& response) const = 0;
        
        // Property has to check 
	// virtual uint32_t Status(const string& index /* @index */, IMetaDataServiceIterator*& response /* @out */) const = 0;
        // virtual uint32_t Links(IMetaDataChannelIterator*& response /* @out */) const = 0;
        // virtual uint32_t ProcessInfo(PluginHost::MetaData::Server& response /* @out */) const = 0;
        // virtual uint32_t DiscoveryResults(IMetaDataBridgeIterator*& response) const;

        // @property
        virtual uint32_t Environment(const string& index /* @index */, string& response /* @out */) const = 0;
        // @property
        virtual uint32_t Configuration(const string& index /* @index */, string& response /* @out */) const = 0;
        // @property
        virtual uint32_t Configuration(const string& index /* @index */, const string& response /* @in */) = 0;
    };
}
}
