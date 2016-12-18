// This file is part of boinc-shmem-tool.
// https://github.com/boinc-next/boinc-shmem-tool
// Copyright (C) 2016-2017 Artem Vorotnikov
//
// boinc-shmem-tool is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// boinc-shmem-toolis distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with boinc-shmem-tool.  If not, see <http://www.gnu.org/licenses/>.

#include <json/json.h>
namespace API {
extern "C" {
    #include <boinc-app-api/api.h>
}
}

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <optional>

using namespace std;
namespace po = boost::program_options;

const auto ACTION_OPTION = "action";
const auto CHANNEL_OPTION = "channel";
const auto DATA_OPTION = "result";
const auto PAYLOAD_OPTION = "payload";

enum class Channel {
    ProcessControlRequest,
    ProcessControlReply,
    GraphicsRequest,
    GraphicsReply,
    Heartbeat,
    TrickleUp,
    TrickleDown,
    AppStatus,
};

auto parse_channel_string(const string& v) -> optional<Channel> {
    if (0) {}
    else if (v == "process_control_request") { return Channel::ProcessControlRequest; }
    else if (v == "process_control_reply") { return Channel::ProcessControlReply; }
    else if (v == "graphics_request") { return Channel::GraphicsRequest; }
    else if (v == "graphics_reply") { return Channel::GraphicsReply; }
    else if (v == "heartbeat") { return Channel::Heartbeat; }
    else if (v == "trickle_up") { return Channel::TrickleUp; }
    else if (v == "trickle_down") { return Channel::TrickleDown; }
    else if (v == "app_status") { return Channel::AppStatus; }
    else { return nullopt; }
}

auto into_string(const Channel& v) -> string {
    switch (v) {
        case Channel::ProcessControlRequest:
            return "process_control_request";
        case Channel::ProcessControlReply:
            return "process_control_reply";
        case Channel::GraphicsRequest:
            return "graphics_request";
        case Channel::GraphicsReply:
            return "graphics_reply";
        case Channel::Heartbeat:
            return "heartbeat";
        case Channel::TrickleUp:
            return "trickle_up";
        case Channel::TrickleDown:
            return "trickle_down";
        case Channel::AppStatus:
            return "app_status";
        default:
            throw;
    }
}

enum class Action {
    View,
    Receive,
    Delete,
    Send,
    Force,
};

auto parse_action_string(const string& v) -> optional<Action> {
    if (0) {}
    else if (v == "view") { return Action::View; }
    else if (v == "receive") { return Action::Receive; }
    else if (v == "send") { return Action::Send; }
    else if (v == "delete") { return Action::Delete; }
    else if (v == "force") { return Action::Force; }
    else { return nullopt; }
}

auto into_string(const Action& v) -> string {
    switch (v) {
        case Action::View:
            return "view";
        case Action::Receive:
            return "receive";
        case Action::Delete:
            return "send";
        case Action::Send:
            return "delete";
        case Action::Force:
            return "force";
        default:
            throw;
    }
}

struct Request {
    Action action;
    Channel channel;
    string payload;
    Request(const string& in) {
        Json::Value v(Json::objectValue);
        auto parsed = Json::Reader().parse(in, v);
        if (not parsed) {
            throw invalid_argument("Invalid JSON");
        }
        if (v.type() != Json::objectValue) {
            throw invalid_argument("Invalid JSON");
        }

        action = parse_action_string(v[ACTION_OPTION].asString()).value();
        channel = parse_channel_string(v[CHANNEL_OPTION].asString()).value();
        payload = v[PAYLOAD_OPTION].asString();
    }
};

auto get_channel(API::SHARED_MEM& shmem, Channel id) -> API::MSG_CHANNEL& {
    switch (id) {
        case Channel::ProcessControlRequest: return shmem.process_control_request;
        case Channel::ProcessControlReply: return shmem.process_control_reply;
        case Channel::GraphicsRequest: return shmem.graphics_request;
        case Channel::GraphicsReply: return shmem.graphics_reply;
        case Channel::Heartbeat: return shmem.heartbeat;
        case Channel::TrickleUp: return shmem.trickle_up;
        case Channel::TrickleDown: return shmem.trickle_down;
        case Channel::AppStatus: return shmem.app_status;
        default: throw invalid_argument("No such channel.");
    }
}

auto chan_peek(API::MSG_CHANNEL& chan) -> optional<string> {
    char buf[MSG_CHANNEL_SIZE];
    if (not API::msg_channel_peek_msg(&chan, buf)) {
        return nullopt;
    }

    return buf;
}

auto chan_recv(API::MSG_CHANNEL& chan) -> optional<string> {
    char buf[MSG_CHANNEL_SIZE];
    if (not API::msg_channel_get_msg(&chan, buf)) {
        return nullopt;
    }

    return buf;
}

auto do_action(
    API::SHARED_MEM& shmem,
    Action action,
    Channel channel,
    string payload=""
) -> Json::Value {
    Json::Value v;
    auto& chan = get_channel(shmem, channel);
    switch (action) {
        case Action::View:
            try {
                v["message"] = chan_peek(chan).value();
                v["ok"] = true;
            } catch (const bad_optional_access& e) {
                v["ok"] = false;
            }
            break;
        case Action::Receive:
            try {
                v["message"] = chan_recv(chan).value();
                v["ok"] = true;
            } catch (const bad_optional_access& e) {
                v["ok"] = false;
            }
            break;
        case Action::Delete:
            API::msg_channel_delete_msg(&chan);
            v["ok"] = true;
            break;
        case Action::Send:
            if (payload.empty()) throw invalid_argument("Send requires payload.");
            if (API::msg_channel_send_msg(&chan, payload.c_str())) {
                v["ok"] = true;
            } else {
                v["ok"] = false;
            }
            break;
        case Action::Force:
            if (payload.empty()) throw invalid_argument("Force requires payload.");
            API::msg_channel_send_msg_overwrite(&chan, payload.c_str());
            v["ok"] = true;
            break;
        default:
            throw invalid_argument("Action not implemented");
    }
    v["action"] = into_string(action);
    v["channel"] = into_string(channel);

    return v;
}

struct SharedMemoryMap {
    bool managed;
    string mmap_path;
    API::SHARED_MEM* data;
    SharedMemoryMap(string mpath, bool manage) {
        this->mmap_path = mpath;
        this->managed = manage;
        this->data = nullptr;

        void** pp = (void**)&this->data;
        int fd = manage ? open(mpath.data(), O_RDWR | O_CREAT, 0666) : open(mpath.data(), O_RDWR);
        if (fd < 0) {
            throw std::runtime_error("Failed to open file descriptor");
        }

        struct stat sbuf;
        if (auto err = fstat(fd, &sbuf); err != 0) {
            close(fd);
            throw runtime_error((boost::format("fstat() failed: %1%") % err).str());
        }
        int size = sizeof(API::SHARED_MEM);
        if (!manage) {
            if (sbuf.st_size == 0) {
                close(fd);
                throw runtime_error("Allocated mmap size is 0");
            }
        } else {
            if (sbuf.st_size < size) {
                // The following 2 lines extend the file and clear its new 
                // area to all zeros because they write beyond the old EOF. 
                // See the lseek man page for details.
                lseek(fd, size-1, SEEK_SET);
                write(fd, "\0", 1);
            }
        }

        *pp = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, fd, 0);

        close(fd);

        if (*pp == MAP_FAILED) {
            throw std::runtime_error("mmap() failed to map against SHARED_MEM");
        }
    }
    ~SharedMemoryMap() {
        if (this->managed) {
            munmap(this->data, sizeof(API::SHARED_MEM));
        }
    }
};

auto main(int argc, char** argv) -> int {
    po::options_description desc("");
    desc.add_options()
        ("help", "produce help message")
        ("manage", po::value<bool>(), "Create and delete mmap file")
        ("mmap-file", po::value<string>(), "mmap file location")
    ;

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
    
    po::notify(vm);
    
    if (vm.count("help")) {
        cout << desc << "\n";
        return 1;
    }

    if (!vm.count("mmap-file")) {
        cout << "Please specify mmap-file" << endl;
        return 1;
    }

    auto mpath = vm["mmap-file"].as<string>();

    auto manage = false;
    if (vm.count("manage")) {
        manage = vm["manage"].as<bool>();
    }

    std::unique_ptr<SharedMemoryMap> shmem_result;
    try {
        shmem_result = make_unique<SharedMemoryMap>(mpath, manage);
    } catch (const exception& e) {
        cerr << e.what() << endl;
        return 1;
    }

    auto& shmem = *shmem_result->data;

    while(true) {
        Json::Value v;
        try {
            string req_str;
            if (not getline(cin, req_str)) break;
            Request req(req_str);

            v = do_action(shmem, req.action, req.channel, req.payload);
        } catch (exception& e) {
            v["error"] = e.what();
        }
        cout << Json::FastWriter().write(v);
    }
}
