/*
 * Copyright (c) 2026 Rally Tactical Systems, Inc.
 * All rights reserved.
 *
 * Generate UTF-8 XML payloads for Cursor on Target (CoT) transmission.
 * Companion to generate_cot_payload.py: same CLI flags and compact XML shape.
 */

#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{

const char kDefaultRemarksJson[] =
    "{\"note\":\"CoT sample payload\",\"source\":\"cotmon\"}";

std::string escapeXmlAttr(const std::string& s)
{
    std::string out;
    out.reserve(s.size() + 8U);
    for (unsigned char c : s)
    {
        switch (c)
        {
        case '&':
            out += "&amp;";
            break;
        case '"':
            out += "&quot;";
            break;
        case '<':
            out += "&lt;";
            break;
        default:
            out += static_cast<char>(c);
            break;
        }
    }
    return out;
}

std::string escapeXmlElemText(const std::string& s)
{
    std::string out;
    out.reserve(s.size() + 8U);
    for (unsigned char c : s)
    {
        switch (c)
        {
        case '&':
            out += "&amp;";
            break;
        case '<':
            out += "&lt;";
            break;
        case '>':
            out += "&gt;";
            break;
        default:
            out += static_cast<char>(c);
            break;
        }
    }
    return out;
}

std::string trim(const std::string& s)
{
    size_t a = 0;
    while (a < s.size() && std::isspace(static_cast<unsigned char>(s[a])))
    {
        ++a;
    }
    size_t b = s.size();
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1])))
    {
        --b;
    }
    return s.substr(a, b - a);
}

std::string isoUtcMillis(std::chrono::system_clock::time_point tp)
{
    using namespace std::chrono;
    const int64_t ms = duration_cast<milliseconds>(tp.time_since_epoch()).count();
    const time_t sec = static_cast<time_t>(ms / 1000);
    const int millis = static_cast<int>(ms % 1000);
    struct tm tmUtc;
#if defined(_WIN32)
    gmtime_s(&tmUtc, &sec);
#else
    gmtime_r(&sec, &tmUtc);
#endif
    char buf[40];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tmUtc);
    char out[56];
    std::snprintf(out, sizeof(out), "%s.%03dZ", buf, millis);
    return std::string(out);
}

void skipWs(const char*& p)
{
    while (*p != '\0' && std::isspace(static_cast<unsigned char>(*p)))
    {
        ++p;
    }
}

bool isNameChar(unsigned char c)
{
    return std::isalnum(c) != 0 || c == '_' || c == '-' || c == '.';
}

std::string parseName(const char*& p)
{
    const char* start = p;
    while (isNameChar(static_cast<unsigned char>(*p)))
    {
        ++p;
    }
    return std::string(start, p);
}

struct XmlNode
{
    std::string name;
    std::vector<std::pair<std::string, std::string> > attrs;
    std::string text;
    std::vector<XmlNode> children;
};

bool parseXmlElement(const char*& p, XmlNode& node, std::string& err);

bool parseXmlBodyRoot(const std::string& body, XmlNode& root, std::string& err)
{
    const char* p = body.c_str();
    skipWs(p);
    if (*p != '<')
    {
        err = "expected root element";
        return false;
    }
    return parseXmlElement(p, root, err);
}

bool parseXmlElement(const char*& p, XmlNode& node, std::string& err)
{
    if (*p != '<')
    {
        err = "expected '<'";
        return false;
    }
    ++p;
    if (*p == '/')
    {
        err = "unexpected end tag";
        return false;
    }
    node.name = parseName(p);
    if (node.name.empty())
    {
        err = "empty element name";
        return false;
    }
    skipWs(p);
    while (*p != '\0' && *p != '>' && !(*p == '/' && p[1] == '>'))
    {
        std::string an = parseName(p);
        if (an.empty())
        {
            err = "bad attribute name";
            return false;
        }
        skipWs(p);
        if (*p != '=')
        {
            err = "expected '=' after attribute";
            return false;
        }
        ++p;
        skipWs(p);
        if (*p != '"')
        {
            err = "expected '\"' for attribute value";
            return false;
        }
        ++p;
        std::string val;
        while (*p != '\0' && *p != '"')
        {
            if (*p == '&')
            {
                if (std::strncmp(p, "&amp;", 5) == 0)
                {
                    val += '&';
                    p += 5;
                }
                else if (std::strncmp(p, "&quot;", 6) == 0)
                {
                    val += '"';
                    p += 6;
                }
                else if (std::strncmp(p, "&lt;", 4) == 0)
                {
                    val += '<';
                    p += 4;
                }
                else if (std::strncmp(p, "&gt;", 4) == 0)
                {
                    val += '>';
                    p += 4;
                }
                else
                {
                    val += *p++;
                }
            }
            else
            {
                val += *p++;
            }
        }
        if (*p != '"')
        {
            err = "unterminated attribute";
            return false;
        }
        ++p;
        node.attrs.push_back(std::make_pair(an, val));
        skipWs(p);
    }
    if (*p == '/' && p[1] == '>')
    {
        p += 2;
        return true;
    }
    if (*p != '>')
    {
        err = "expected '>' or '/>'";
        return false;
    }
    ++p;

    for (;;)
    {
        skipWs(p);
        if (*p == '\0')
        {
            err = "unexpected end inside <" + node.name + ">";
            return false;
        }
        if (*p != '<')
        {
            const char* t = p;
            while (*p != '\0' && *p != '<')
            {
                ++p;
            }
            node.text.append(t, static_cast<size_t>(p - t));
            continue;
        }
        if (p[1] == '/')
        {
            const char* q = p + 2;
            std::string closeName = parseName(q);
            skipWs(q);
            if (closeName != node.name)
            {
                err = "mismatched close tag";
                return false;
            }
            if (*q != '>')
            {
                err = "expected '>' after close tag";
                return false;
            }
            p = q + 1;
            return true;
        }
        XmlNode child;
        if (!parseXmlElement(p, child, err))
        {
            return false;
        }
        node.children.push_back(child);
    }
}

std::string repeatIndent(const std::string& indent, int depth)
{
    std::string s;
    for (int i = 0; i < depth; ++i)
    {
        s += indent;
    }
    return s;
}

bool textHasNonSpace(const std::string& th)
{
    for (size_t i = 0; i < th.size(); ++i)
    {
        if (!std::isspace(static_cast<unsigned char>(th[i])))
        {
            return true;
        }
    }
    return false;
}

void serializePretty(const XmlNode& elem, const std::string& indent, int depth,
    std::vector<std::string>& lines)
{
    const std::string pad = repeatIndent(indent, depth);
    const std::string ip = repeatIndent(indent, depth + 1);

    const bool hasKids = !elem.children.empty();
    const std::string& th = elem.text;
    const bool textNonEmpty = textHasNonSpace(th);
    const bool selfClosing = !hasKids && !textNonEmpty;

    if (!elem.attrs.empty())
    {
        lines.push_back(pad + "<" + elem.name);
        for (size_t i = 0; i < elem.attrs.size(); ++i)
        {
            const std::string& k = elem.attrs[i].first;
            const std::string& v = elem.attrs[i].second;
            const std::string esc = escapeXmlAttr(v);
            const bool last = (i + 1U == elem.attrs.size());
            if (last && selfClosing)
            {
                lines.push_back(ip + k + "=\"" + esc + "\" />");
            }
            else if (last)
            {
                lines.push_back(ip + k + "=\"" + esc + "\">");
            }
            else
            {
                lines.push_back(ip + k + "=\"" + esc + "\"");
            }
        }
    }
    else if (selfClosing)
    {
        lines.push_back(pad + "<" + elem.name + " />");
    }
    else
    {
        lines.push_back(pad + "<" + elem.name + ">");
    }

    if (selfClosing)
    {
        return;
    }

    if (textNonEmpty)
    {
        std::istringstream iss(th);
        std::string line;
        while (std::getline(iss, line))
        {
            size_t a = 0;
            while (a < line.size() && std::isspace(static_cast<unsigned char>(line[a])))
            {
                ++a;
            }
            std::string stripped = line.substr(a);
            while (!stripped.empty() &&
                std::isspace(static_cast<unsigned char>(stripped.back())))
            {
                stripped.erase(stripped.size() - 1U);
            }
            if (!stripped.empty())
            {
                lines.push_back(ip + escapeXmlElemText(stripped));
            }
        }
    }

    for (size_t c = 0; c < elem.children.size(); ++c)
    {
        serializePretty(elem.children[c], indent, static_cast<int>(depth + 1), lines);
    }

    lines.push_back(pad + "</" + elem.name + ">");
}

void splitXmlDeclaration(const std::string& xml, std::string& decl, std::string& body)
{
    decl.clear();
    const std::string s = trim(xml);
    if (s.size() >= 5U && s.compare(0, 5, "<?xml") == 0)
    {
        const size_t end = s.find("?>");
        if (end != std::string::npos)
        {
            decl = s.substr(0, end + 2U);
            body = trim(s.substr(end + 2U));
            return;
        }
    }
    body = s;
}

std::string humanReadableXml(const std::string& xmlCompact, const std::string& indent)
{
    std::string decl;
    std::string body;
    splitXmlDeclaration(xmlCompact, decl, body);
    XmlNode root;
    std::string err;
    if (!parseXmlBodyRoot(body, root, err))
    {
        return std::string();
    }
    std::vector<std::string> lines;
    serializePretty(root, indent, 0, lines);
    std::ostringstream oss;
    for (size_t i = 0; i < lines.size(); ++i)
    {
        if (i > 0U)
        {
            oss << '\n';
        }
        oss << lines[i];
    }
    oss << '\n';
    std::string formatted = oss.str();
    if (!decl.empty())
    {
        return decl + "\n" + formatted;
    }
    return formatted;
}

std::string xmlDeclaration(bool styleNew)
{
    if (styleNew)
    {
        return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>";
    }
    return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
}

void appendDetailStringElements(std::vector<std::string>& parts,
    const std::vector<std::pair<std::string, std::string> >& pairs)
{
    for (size_t i = 0; i < pairs.size(); ++i)
    {
        const std::string& k = pairs[i].first;
        const std::string& v = pairs[i].second;
        parts.push_back("<" + k + ">" + escapeXmlElemText(v) + "</" + k + ">");
    }
}

bool skipJsonString(const char*& p, std::string& err)
{
    if (*p != '"')
    {
        err = "expected string";
        return false;
    }
    ++p;
    while (*p != '\0')
    {
        if (*p == '\\')
        {
            ++p;
            if (*p == '\0')
            {
                err = "bad escape";
                return false;
            }
            ++p;
            continue;
        }
        if (*p == '"')
        {
            ++p;
            return true;
        }
        ++p;
    }
    err = "unterminated string";
    return false;
}

bool skipJsonValue(const char*& p, std::string& err);

bool skipJsonObject(const char*& p, std::string& err)
{
    if (*p != '{')
    {
        err = "expected '{'";
        return false;
    }
    ++p;
    for (;;)
    {
        skipWs(p);
        if (*p == '}')
        {
            ++p;
            return true;
        }
        if (*p != '"')
        {
            err = "object key must be a string";
            return false;
        }
        if (!skipJsonString(p, err))
        {
            return false;
        }
        skipWs(p);
        if (*p != ':')
        {
            err = "expected ':' after object key";
            return false;
        }
        ++p;
        if (!skipJsonValue(p, err))
        {
            return false;
        }
        skipWs(p);
        if (*p == ',')
        {
            ++p;
            continue;
        }
        if (*p == '}')
        {
            ++p;
            return true;
        }
        err = "expected ',' or '}' in object";
        return false;
    }
}

bool skipJsonArray(const char*& p, std::string& err)
{
    if (*p != '[')
    {
        err = "expected '['";
        return false;
    }
    ++p;
    for (;;)
    {
        skipWs(p);
        if (*p == ']')
        {
            ++p;
            return true;
        }
        if (!skipJsonValue(p, err))
        {
            return false;
        }
        skipWs(p);
        if (*p == ',')
        {
            ++p;
            continue;
        }
        if (*p == ']')
        {
            ++p;
            return true;
        }
        err = "expected ',' or ']' in array";
        return false;
    }
}

bool skipJsonValue(const char*& p, std::string& err)
{
    skipWs(p);
    if (*p == '"')
    {
        return skipJsonString(p, err);
    }
    if (*p == '{')
    {
        return skipJsonObject(p, err);
    }
    if (*p == '[')
    {
        return skipJsonArray(p, err);
    }
    if (*p == 't' && std::strncmp(p, "true", 4) == 0)
    {
        p += 4;
        return true;
    }
    if (*p == 'f' && std::strncmp(p, "false", 5) == 0)
    {
        p += 5;
        return true;
    }
    if (*p == 'n' && std::strncmp(p, "null", 4) == 0)
    {
        p += 4;
        return true;
    }
    if (*p == '-' || std::isdigit(static_cast<unsigned char>(*p)))
    {
        if (*p == '-')
        {
            ++p;
        }
        while (std::isdigit(static_cast<unsigned char>(*p)))
        {
            ++p;
        }
        if (*p == '.')
        {
            ++p;
            while (std::isdigit(static_cast<unsigned char>(*p)))
            {
                ++p;
            }
        }
        if (*p == 'e' || *p == 'E')
        {
            ++p;
            if (*p == '+' || *p == '-')
            {
                ++p;
            }
            while (std::isdigit(static_cast<unsigned char>(*p)))
            {
                ++p;
            }
        }
        return true;
    }
    err = "unsupported JSON value";
    return false;
}

bool decodeJsonStringContent(const char*& p, std::string& out, std::string& err)
{
    out.clear();
    if (*p != '"')
    {
        err = "expected '\"'";
        return false;
    }
    ++p;
    while (*p != '\0')
    {
        if (*p == '\\')
        {
            ++p;
            if (*p == '\0')
            {
                err = "bad escape";
                return false;
            }
            switch (*p)
            {
            case '"':
                out += '"';
                break;
            case '\\':
                out += '\\';
                break;
            case '/':
                out += '/';
                break;
            case 'b':
                out += '\b';
                break;
            case 'f':
                out += '\f';
                break;
            case 'n':
                out += '\n';
                break;
            case 'r':
                out += '\r';
                break;
            case 't':
                out += '\t';
                break;
            case 'u':
                err = "\\u not supported in detail-json";
                return false;
            default:
                out += *p;
                break;
            }
            ++p;
            continue;
        }
        if (*p == '"')
        {
            ++p;
            return true;
        }
        out += *p++;
    }
    err = "unterminated string";
    return false;
}

bool parseDetailJsonPairs(const std::string& j,
    std::vector<std::pair<std::string, std::string> >& out, std::string& err)
{
    out.clear();
    const char* p = j.c_str();
    skipWs(p);
    if (*p != '{')
    {
        err = "--detail-json must be a JSON object";
        return false;
    }
    ++p;
    for (;;)
    {
        skipWs(p);
        if (*p == '}')
        {
            ++p;
            skipWs(p);
            if (*p != '\0')
            {
                err = "trailing junk in detail-json";
                return false;
            }
            return true;
        }
        std::string key;
        if (!decodeJsonStringContent(p, key, err))
        {
            return false;
        }
        skipWs(p);
        if (*p != ':')
        {
            err = "expected ':'";
            return false;
        }
        ++p;
        skipWs(p);
        if (*p == '"')
        {
            std::string val;
            if (!decodeJsonStringContent(p, val, err))
            {
                return false;
            }
            out.push_back(std::make_pair(key, val));
        }
        else
        {
            if (!skipJsonValue(p, err))
            {
                return false;
            }
        }
        skipWs(p);
        if (*p == ',')
        {
            ++p;
            continue;
        }
        if (*p == '}')
        {
            ++p;
            skipWs(p);
            if (*p != '\0')
            {
                err = "trailing junk in detail-json";
                return false;
            }
            return true;
        }
        err = "expected ',' or '}'";
        return false;
    }
}

std::string buildCotXml(bool styleNew, const std::string& uid, const std::string& cotType,
    const std::string& how, const std::string& lat, const std::string& lon,
    const std::string& ce, const std::string& hae, const std::string& le,
    int staleOffsetSecs, const std::string* remarks, const std::string* callsign,
    const std::string* endpoint,
    const std::vector<std::pair<std::string, std::string> >* detailExtra)
{
    using namespace std::chrono;
    const system_clock::time_point t0 = system_clock::now();
    const system_clock::time_point stale = t0 + seconds(staleOffsetSecs);
    const std::string nowS = isoUtcMillis(t0);
    const std::string staleS = isoUtcMillis(stale);

    std::vector<std::string> parts;
    parts.reserve(16U);
    parts.push_back(xmlDeclaration(styleNew));
    parts.push_back(std::string("<event version=\"2.0\" uid=\"") + escapeXmlAttr(uid) +
        "\" type=\"" + escapeXmlAttr(cotType) + "\" time=\"" + escapeXmlAttr(nowS) +
        "\" start=\"" + escapeXmlAttr(nowS) + "\" stale=\"" + escapeXmlAttr(staleS) +
        "\" how=\"" + escapeXmlAttr(how) + "\">");
    parts.push_back(std::string("<point lat=\"") + escapeXmlAttr(lat) + "\" lon=\"" +
        escapeXmlAttr(lon) + "\" ce=\"" + escapeXmlAttr(ce) + "\" hae=\"" +
        escapeXmlAttr(hae) + "\" le=\"" + escapeXmlAttr(le) + "\"/>");
    parts.push_back("<detail>");

    if (callsign != 0)
    {
        std::string c =
            std::string("<contact callsign=\"") + escapeXmlAttr(*callsign) + "\"";
        if (endpoint != 0)
        {
            c += " endpoint=\"" + escapeXmlAttr(*endpoint) + "\"";
        }
        c += "/>";
        parts.push_back(c);
    }

    if (remarks != 0 && !remarks->empty())
    {
        parts.push_back(std::string("<remarks>") + escapeXmlElemText(*remarks) + "</remarks>");
    }

    if (detailExtra != 0)
    {
        appendDetailStringElements(parts, *detailExtra);
    }

    parts.push_back("</detail>");
    parts.push_back("</event>");

    std::string out;
    for (size_t i = 0; i < parts.size(); ++i)
    {
        out += parts[i];
    }
    return out;
}

bool readFileUtf8(const std::string& path, std::string& out, std::string& err)
{
    std::ifstream f(path.c_str(), std::ios::binary);
    if (!f)
    {
        err = "cannot open " + path;
        return false;
    }
    std::ostringstream oss;
    oss << f.rdbuf();
    out = oss.str();
    return true;
}

void die(const std::string& msg)
{
    std::fprintf(stderr, "%s\n", msg.c_str());
    std::exit(1);
}

struct Options
{
    bool formatNew;
    bool noRemarks;
    bool pretty;
    int staleOffsetSecs;
    std::string uid;
    std::string cotType;
    std::string how;
    std::string lat;
    std::string lon;
    std::string ce;
    std::string hae;
    std::string le;
    std::string callsign;
    std::string endpoint;
    std::string detailJson;
    std::string indent;
    std::string outputFile;
    bool hasPayloadJson;
    std::string payloadJson;
    bool hasPayloadFile;
    std::string payloadFile;
};

bool streq(const char* a, const char* b)
{
    return std::strcmp(a, b) == 0;
}

void parseArgs(int argc, char** argv, Options& o)
{
    o.formatNew = true;
    o.noRemarks = false;
    o.pretty = false;
    o.staleOffsetSecs = 300;
    o.uid = "cotmon-uid";
    o.cotType = "a-f-G-U-C";
    o.how = "h-e";
    o.lat = "0.0";
    o.lon = "0.0";
    o.ce = "9999999";
    o.hae = "0";
    o.le = "9999999";
    o.indent = "  ";
    o.hasPayloadJson = false;
    o.hasPayloadFile = false;

    for (int i = 1; i < argc; ++i)
    {
        const char* arg = argv[i];
        if (streq(arg, "--format") && i + 1 < argc)
        {
            ++i;
            if (streq(argv[i], "original"))
            {
                o.formatNew = false;
            }
            else if (streq(argv[i], "new"))
            {
                o.formatNew = true;
            }
            else
            {
                die("usage: --format original|new");
            }
        }
        else if (streq(arg, "--payload-json") && i + 1 < argc)
        {
            o.hasPayloadJson = true;
            o.payloadJson = argv[++i];
        }
        else if (streq(arg, "--payload-file") && i + 1 < argc)
        {
            o.hasPayloadFile = true;
            o.payloadFile = argv[++i];
        }
        else if (streq(arg, "--no-remarks"))
        {
            o.noRemarks = true;
        }
        else if (streq(arg, "--uid") && i + 1 < argc)
        {
            o.uid = argv[++i];
        }
        else if (streq(arg, "--type") && i + 1 < argc)
        {
            o.cotType = argv[++i];
        }
        else if (streq(arg, "--how") && i + 1 < argc)
        {
            o.how = argv[++i];
        }
        else if (streq(arg, "--lat") && i + 1 < argc)
        {
            o.lat = argv[++i];
        }
        else if (streq(arg, "--lon") && i + 1 < argc)
        {
            o.lon = argv[++i];
        }
        else if (streq(arg, "--ce") && i + 1 < argc)
        {
            o.ce = argv[++i];
        }
        else if (streq(arg, "--hae") && i + 1 < argc)
        {
            o.hae = argv[++i];
        }
        else if (streq(arg, "--le") && i + 1 < argc)
        {
            o.le = argv[++i];
        }
        else if (streq(arg, "--stale-offset-secs") && i + 1 < argc)
        {
            o.staleOffsetSecs = std::atoi(argv[++i]);
        }
        else if (streq(arg, "--callsign") && i + 1 < argc)
        {
            o.callsign = argv[++i];
        }
        else if (streq(arg, "--endpoint") && i + 1 < argc)
        {
            o.endpoint = argv[++i];
        }
        else if (streq(arg, "--detail-json") && i + 1 < argc)
        {
            o.detailJson = argv[++i];
        }
        else if (streq(arg, "--pretty") || streq(arg, "--human-readable"))
        {
            o.pretty = true;
        }
        else if (streq(arg, "--indent") && i + 1 < argc)
        {
            o.indent = argv[++i];
        }
        else if ((streq(arg, "-o") || streq(arg, "--output-file")) && i + 1 < argc)
        {
            o.outputFile = argv[++i];
        }
        else if (streq(arg, "-h") || streq(arg, "--help"))
        {
            std::fprintf(stderr,
                "Usage: generate_cot_payload [options]\n"
                "  --format original|new   (default: new)\n"
                "  --payload-json STR | --payload-file PATH\n"
                "  --no-remarks\n"
                "  --uid --type --how --lat --lon --ce --hae --le\n"
                "  --stale-offset-secs N   (default: 300)\n"
                "  --callsign --endpoint\n"
                "  --detail-json '{\"k\":\"v\"}'  (string values -> elements)\n"
                "  --pretty | --human-readable   --indent TEXT\n"
                "  -o|--output-file PATH\n");
            std::exit(0);
        }
        else
        {
            die(std::string("unknown argument: ") + arg);
        }
    }

    if (o.hasPayloadJson && o.hasPayloadFile)
    {
        die("use only one of --payload-json and --payload-file");
    }
}

} // namespace

int main(int argc, char** argv)
{
    Options o;
    parseArgs(argc, argv, o);

    if (!o.hasPayloadJson && !o.hasPayloadFile && !o.noRemarks)
    {
        o.hasPayloadJson = true;
        o.payloadJson = kDefaultRemarksJson;
    }

    const std::string* remarksPtr = 0;
    std::string remarksBuf;
    std::string err;

    if (!o.noRemarks)
    {
        if (o.hasPayloadFile)
        {
            if (!readFileUtf8(o.payloadFile, remarksBuf, err))
            {
                die(err);
            }
            remarksPtr = &remarksBuf;
        }
        else if (o.hasPayloadJson)
        {
            remarksBuf = o.payloadJson;
            remarksPtr = &remarksBuf;
        }
    }

    std::vector<std::pair<std::string, std::string> > detailPairs;
    const std::vector<std::pair<std::string, std::string> >* detailPtr = 0;
    if (!trim(o.detailJson).empty())
    {
        if (!parseDetailJsonPairs(o.detailJson, detailPairs, err))
        {
            die(err);
        }
        detailPtr = &detailPairs;
    }

    const std::string csTrim = trim(o.callsign);
    const std::string epTrim = trim(o.endpoint);
    const std::string* callsignPtr =
        csTrim.empty() ? static_cast<const std::string*>(0) : &csTrim;
    const std::string* endpointPtr =
        epTrim.empty() ? static_cast<const std::string*>(0) : &epTrim;

    std::string xml = buildCotXml(o.formatNew, o.uid, o.cotType, o.how, o.lat, o.lon, o.ce,
        o.hae, o.le, o.staleOffsetSecs, remarksPtr, callsignPtr, endpointPtr, detailPtr);

    if (o.pretty)
    {
        const std::string pretty = humanReadableXml(xml, o.indent);
        if (pretty.empty())
        {
            die("pretty-print failed (invalid XML?)");
        }
        xml = pretty;
    }

    if (!o.outputFile.empty())
    {
        std::ofstream out(o.outputFile.c_str(), std::ios::binary);
        if (!out)
        {
            die("cannot open output file");
        }
        out.write(xml.data(), static_cast<std::streamsize>(xml.size()));
    }
    else
    {
        if (std::fwrite(xml.data(), 1U, xml.size(), stdout) != xml.size())
        {
            return 1;
        }
    }
    return 0;
}
