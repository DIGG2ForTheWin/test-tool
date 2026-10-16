#include "metadata.h"
#include "io_util.h"
#include <string>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <cerrno>


namespace  {
    namespace fs = std::filesystem;
}

std::string xml_escape(std::string_view text) {
    std::string result;
    result.reserve(text.size());

    for (const auto c: text) {
        switch(c) {
            case '&': result += "&amp;"; break;
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '"': result += "&quot;"; break;
            case '\'': result += "&apos;"; break;
            default: result += c;
        }
    }
    return result;
}

std::string build_metadata_xml(const MessageHeader& header, std::string_view from, std::string_view to) {
    // Escape every value once, up front. Nothing from the config goes into
    // the XML without passing through xml_escape.
    const std::string party_type    = xml_escape(header.party_id_type);
    const std::string from_id       = xml_escape(from);
    const std::string to_id         = xml_escape(to);
    const std::string sender_role   = xml_escape(header.sender_role);
    const std::string receiver_role = xml_escape(header.receiver_role);
    const std::string service       = xml_escape(header.service);
    const std::string service_type  = xml_escape(header.service_type);
    const std::string action        = xml_escape(header.action);

    // originalSender / finalRecipient are "<party_id_type>:<node name>"
    const std::string original_sender = xml_escape(header.party_id_type + ":" + std::string(from));
    const std::string final_recipient = xml_escape(header.party_id_type + ":" + std::string(to));

    std::ostringstream xml;
    xml << R"(<?xml version="1.0" encoding="UTF-8"?>)" << '\n'
        << R"(<UserMessage xmlns="http://docs.oasis-open.org/ebxml-msg/ebms/v3.0/ns/core/200704/">)" << '\n'
        << "  <PartyInfo>\n"
        << "    <From>\n"
        << R"(      <PartyId type=")" << party_type << R"(">)" << from_id << "</PartyId>\n"
        << "      <Role>" << sender_role << "</Role>\n"
        << "    </From>\n"
        << "    <To>\n"
        << R"(      <PartyId type=")" << party_type << R"(">)" << to_id << "</PartyId>\n"
        << "      <Role>" << receiver_role << "</Role>\n"
        << "    </To>\n"
        << "  </PartyInfo>\n"
        << "  <CollaborationInfo>\n"
        << R"(    <Service type=")" << service_type << R"(">)" << service << "</Service>\n"
        << "    <Action>" << action << "</Action>\n"
        << "  </CollaborationInfo>\n"
        << "  <MessageProperties>\n"
        << R"(    <Property name="originalSender">)" << original_sender << "</Property>\n"
        << R"(    <Property name="finalRecipient">)" << final_recipient << "</Property>\n"
        << "  </MessageProperties>\n"
        << "</UserMessage>\n";

    return xml.str();
}

void write_metadata(const fs::path& dir, const MessageHeader& header, std::string_view from, std::string_view to) {
    
    const std::string metadata = build_metadata_xml(header, from, to);

    const fs::path metadata_file = dir / "metadata.xml";
    std::ofstream out(metadata_file);


    if (!out) {
        const int err = errno;
        fail_and_remove(out, metadata_file,"failed to create file '" + fs::absolute(metadata_file).string() + "': " + std::strerror(err));
    }

    out << metadata;

    out.close();

    if (!out) {
        const int err = errno;
        fail_and_remove(out, metadata_file, "failed to finish writing '" + fs::absolute(metadata_file).string() + "': " + std::strerror(err));
    }
}