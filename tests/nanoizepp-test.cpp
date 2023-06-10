#include <catch2/catch_test_macros.hpp>

#include <nanoizepp/nanoizepp.hpp>

TEST_CASE("BASIC HTML", "[nanoizepp-test]")
{
    std::string html = "<!DOCTYPE html><html><head><title>Test</title></head><body><h1>Test</h1><p>Test</p></body></html>";
    std::string miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == html);


    std::string indented_html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Test</title>
</head>
<body>
    <h1>Test</h1>
    <p>Test</p>
</body>
</html>)";
    miniaturized = nanoizepp::nanoize(indented_html);
    CHECK(miniaturized == html);

    std::string anchor = R"(<a href="https://example.com" target="_blank">Hello World</a>)";
    miniaturized = nanoizepp::nanoize(anchor);
    CHECK(miniaturized == anchor);
}

TEST_CASE("Abrupt closing of comments", "[nanoizepp-test]")
{
    std::string html = R"(<!-- Hello World -->)";
    std::string miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "");

    html = R"(<!-->)";
    miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "");

    html = R"(<!--->)";
    miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "");
}

TEST_CASE("Space collasping", "[nanoizepp-test]")
{
    std::string html = R"(<p>Hello               world</p>)";
    std::string miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "<p>Hello world</p>");

    html = "<p>Hello\n               world</p>";
    miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "<p>Hello world</p>");
}

TEST_CASE("End tags with attributes", "[nanoizepp-test]")
{
    std::string html = R"(<p>123</p class="red">)";
    std::string miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "<p>123</p>");
}

TEST_CASE("Duplicated attributes", "[nanoizepp-test]")
{
    std::string html = R"(<p class="red" class="blue">123</p>)";
    std::string miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "<p class=\"red\">123</p>");
}

TEST_CASE("EOF before tag name", "[nanoizepp-test]")
{
    std::string html = "<";
    std::string miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "<");

    html = "</";
    miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "</");
}

TEST_CASE("Incorrectly closed comments", "[nanoizepp-test]")
{
    std::string html = "<!-- Hello World --!>";
    std::string miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "");
}

TEST_CASE("End tag with tailing solidus")
{
    std::string html = R"(<p>123</p/>)";
    std::string miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "<p>123</p>");
}

TEST_CASE("EOF in comment")
{
    std::string html = "<!--";
    std::string miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "");

    html = "<!---";
    miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "");
}

TEST_CASE("invalid-first-character-of-tag-name")
{
    std::string html = "<42></42>";
    std::string miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "42");
}

TEST_CASE("missing-attribute-value")
{
    std::string html = R"(<p class>123</p>)";
    std::string miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "<p>123</p>");
}

TEST_CASE("missing-end-tag-name")
{
    std::string html = "<p></></p>";
    std::string miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "<p></p>");
}

TEST_CASE("nested-comment")
{
    std::string html = "<p><!-- <!-- --> --></p>";
    std::string miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "<p> --></p>");
}

TEST_CASE("null-character-reference")
{
    std::string html = std::string(1, '\0');
    std::string miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "\uFFFD");
}

TEST_CASE("unexpected-character-in-attribute-name")
{
    std::string html = R"(<div foo=b'ar'>)";
    std::string miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "<div foo=\"b'ar'\"></div>");
}

TEST_CASE("unexpected-character-after-attribute-name")
{
    std::string html = R"(<div id=></div>)";
    std::string miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "<div></div>");
}

TEST_CASE("unexpected-question-mark-instead-of-tag-name")
{
    std::string html = R"(<?xml-stylesheet type="text/css" href="style.css"?>)";
    std::string miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "");
}

TEST_CASE("non-void-html-element-start-tag-with-trailing-solidus")
{
    std::string html = R"(<p/>123</p>)";
    std::string miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "<p>123</p>");
}

TEST_CASE("cdata-in-html-content")
{
    std::string html = R"(<p>123<![CDATA[456]]>789</p>)";
    std::string miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "<p>123789</p>");
}

TEST_CASE("cdata-in-sgv-math")
{
    std::string html = R"(<math><![CDATA[<]]></math>)";
    std::string miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "<math><![CDATA[<]]></math>");

    html = R"(<svg><![CDATA[<]]></svg>)";
    miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "<svg><![CDATA[<]]></svg>");

}

TEST_CASE("Attributes", "[nanoizepp-test]")
{
    std::string html = R"(<div class="main_disp"     ></div>)";
    std::string miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "<div class=\"main_disp\"></div>");
}

TEST_CASE("Slash in tag", "[nanoizepp-test]")
{
    std::string html = R"(<div / class="main_disp"     ></div>)";
    std::string miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "<div class=\"main_disp\"></div>");
}

TEST_CASE("Tags in multiline paragraph", "[nanoizepp-test]")
{
    std::string html = R"(<p>Hello<div class="foo"     >
    Bar</div></p>)";
    std::string miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "<p>Hello<div class=\"foo\"> Bar</div></p>");
}

TEST_CASE("Lorem Ipsum")
{
    std::string html = R"(<p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed non risus. Suspendisse lectus tortor, dignissim sit amet, adipiscing nec, ultricies sed, dolor.</p>)";
    std::string miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "<p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed non risus. Suspendisse lectus tortor, dignissim sit amet, adipiscing nec, ultricies sed, dolor.</p>");
}

TEST_CASE("Complex nesting or tags and text")
{
    std::string html = R"(<a href="https://github.com/marty1885" target="_blank">Github <i class="fa-solid fa-arrow-up-right-from-square fa-xs"></i><div></div></a>)";
    std::string miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == html);
}

TEST_CASE("avoid special tags", "[nanoizepp-test]")
{
    // <pre>, <code>, <textarea>, <plaintext>, <script>, <style>
    std::string html = R"(<pre>    <div class="main_disp"     ></div></pre>)";
    std::string miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "<pre>    <div class=\"main_disp\"     ></div></pre>");

    html = R"(<code>    <div class="main_disp"     ></div></code>)";
    miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "<code>    <div class=\"main_disp\"     ></div></code>");

    html = R"(<textarea>    HELLO WORLD </textarea>)";
    miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "<textarea>    HELLO WORLD </textarea>");

    html = R"(<plaintext>    HELLO WORLD </plaintext>)";
    miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "<plaintext>    HELLO WORLD </plaintext>");

    html = R"(<script> alert("Hello, world!") </script>)";
    miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "<script> alert(\"Hello, world!\") </script>");

    html = R"(<style> .main_disp { color: red; } </style>)";
    miniaturized = nanoizepp::nanoize(html);
    CHECK(miniaturized == "<style> .main_disp { color: red; } </style>");
}