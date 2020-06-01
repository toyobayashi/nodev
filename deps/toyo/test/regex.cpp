// ported from https://github.com/denoland/deno/blob/v1.0.2/std/path/_globrex_test.ts

#include "regex.hpp"
#include "toyo/path.hpp"
#include "cmocha/cmocha.h"

/* function match(
  glob: string,
  strUnix: string,
  strWin?: string | object,
  opts: GlobrexOptions = {}
): boolean {
  if (typeof strWin === "object") {
    opts = strWin;
    strWin = "";
  }
  const { regex } = globrex(glob, opts);
  const match = (isWin && strWin ? strWin : strUnix).match(regex);
  if (match && !regex.flags.includes("g")) {
    assertEquals(match.length, 1);
  }
  return !!match;
} */

static int match(
  const std::string& glob,
  const std::string& strUnix,
  const std::string& strWin,
  const toyo::path::globrex::globrex_options& opts
) {
  toyo::path::globrex g(glob, opts);
  auto regex = g.regex;
  std::vector<std::smatch> match;
  std::smatch sm;
#ifdef _WIN32
  std::string tmpstr = strWin != "" ? strWin : strUnix;
#else
  std::string tmpstr = strUnix;
#endif
  while(tmpstr != "" && std::regex_search(tmpstr, sm, regex)) {
    match.push_back(sm);
    tmpstr = sm.suffix();
  }
  if (match.size() > 0) {
    expect(match.size() == 1);
  }
  return match.size() > 0 ? 0 : 1;
}

static int match(
  const std::string& glob,
  const std::string& strUnix,
  const toyo::path::globrex::globrex_options& opts
) {
  return match(glob, strUnix, "", opts);
}

static int match(
  const std::string& glob,
  const std::string& strUnix
) {
  toyo::path::globrex::globrex_options opts;
  return match(glob, strUnix, opts);
}

static int match(
  const std::string& glob,
  const std::string& strUnix,
  const std::string& strWin
) {
  toyo::path::globrex::globrex_options opts;
  return match(glob, strUnix, strWin, opts);
}

int test_globrex() {
  using namespace toyo;
  path::globrex res("*.js");
  expect(res.regex_str == "^.*\\.js$");

  expect(match("*", "foo") == 0);
  expect(match("f*", "foo") == 0);
  expect(match("*o", "foo") == 0);
  expect(match("u*orn", "unicorn") == 0);
  expect(match("ico", "unicorn") == 1);
  expect(match("u*nicorn", "unicorn") == 0);

  toyo::path::globrex::globrex_options opts1;
  opts1.globstar = false;
  expect(match("*.min.js", "http://example.com/jquery.min.js", opts1) == 0);
  expect(match("*.min.*", "http://example.com/jquery.min.js", opts1) == 0);
  expect(match("*/js/*.js", "http://example.com/js/jquery.min.js", opts1) == 0);

  std::string str = "\\/$^+?.()=!|{},[].*";
  expect(match(str, str) == 0);

  expect(match(".min.", "http://example.com/jquery.min.js") == 1);
  expect(match("*.min.*", "http://example.com/jquery.min.js") == 0);
  expect(match("http:", "http://example.com/jquery.min.js") == 1);
  expect(match("http:*", "http://example.com/jquery.min.js") == 0);
  expect(match("min.js", "http://example.com/jquery.min.js") == 1);
  expect(match("*min.js", "http://example.com/jquery.min.js") == 0);
  expect(match("/js*jq*.js", "http://example.com/js/jquery.min.js") == 1);

  toyo::path::globrex::globrex_options opts2;
  opts2.extended = true;

  expect(match("f?o", "foo", opts2) == 0);
  expect(match("f?o", "fooo", opts2) == 1);
  expect(match("f?oo", "foo", opts2) == 1);

  expect(match("fo[oz]", "foo", opts2) == 0);
  expect(match("fo[oz]", "foz", opts2) == 0);
  expect(match("fo[oz]", "fog", opts2) == 1);
  expect(match("fo[a-z]", "fob", opts2) == 0);
  expect(match("fo[a-d]", "fot", opts2) == 1);
  expect(match("fo[!tz]", "fot", opts2) == 1);
  expect(match("fo[!tz]", "fob", opts2) == 0);

  expect(match("[[:alnum:]]/bar.txt", "a/bar.txt", opts2) == 0);
  expect(match("@([[:alnum:]abc]|11)/bar.txt", "11/bar.txt", opts2) == 0);
  expect(match("@([[:alnum:]abc]|11)/bar.txt", "a/bar.txt", opts2) == 0);
  expect(match("@([[:alnum:]abc]|11)/bar.txt", "b/bar.txt", opts2) == 0);
  expect(match("@([[:alnum:]abc]|11)/bar.txt", "c/bar.txt", opts2) == 0);
  expect(match("@([[:alnum:]abc]|11)/bar.txt", "abc/bar.txt", opts2) == 1);
  expect(match("@([[:alnum:]abc]|11)/bar.txt", "3/bar.txt", opts2) == 0);
  expect(match("[[:digit:]]/bar.txt", "1/bar.txt", opts2) == 0);
  expect(match("[[:digit:]b]/bar.txt", "b/bar.txt", opts2) == 0);
  expect(match("[![:digit:]b]/bar.txt", "a/bar.txt", opts2) == 0);
  expect(match("[[:alnum:]]/bar.txt", "!/bar.txt", opts2) == 1);
  expect(match("[[:digit:]]/bar.txt", "a/bar.txt", opts2) == 1);
  expect(match("[[:digit:]b]/bar.txt", "a/bar.txt", opts2) == 1);

  expect(match("foo{bar,baaz}", "foobaaz", opts2) == 0);
  expect(match("foo{bar,baaz}", "foobar", opts2) == 0);
  expect(match("foo{bar,baaz}", "foobuzz", opts2) == 1);
  expect(match("foo{bar,b*z}", "foobuzz", opts2) == 0);

  expect(match("http://?o[oz].b*z.com/{*.js,*.html}", "http://foo.baaz.com/jquery.min.js", opts2) == 0);
  expect(match("http://?o[oz].b*z.com/{*.js,*.html}", "http://moz.buzz.com/index.html", opts2) == 0);
  expect(match("http://?o[oz].b*z.com/{*.js,*.html}", "http://moz.buzz.com/index.htm", opts2) == 1);
  expect(match("http://?o[oz].b*z.com/{*.js,*.html}", "http://moz.bar.com/index.html", opts2) == 1);
  expect(match("http://?o[oz].b*z.com/{*.js,*.html}", "http://flozz.buzz.com/index.html", opts2) == 1);

  std::string testExtStr = "\\/$^+.()=!|,.*";
  expect(match(testExtStr, testExtStr, opts2) == 0);

  toyo::path::globrex::globrex_options opts3;
  opts3.globstar = true;
  expect(match("/foo/*", "/foo/bar.txt", opts3) == 0);
  expect(match("/foo/**", "/foo/bar.txt", opts3) == 0);
  expect(match("/foo/**", "/foo/bar/baz.txt", opts3) == 0);
  expect(match("/foo/**", "/foo/bar/baz.txt", opts3) == 0);
  expect(match("/foo/*/*.txt", "/foo/bar/baz.txt", opts3) == 0);
  expect(match("/foo/**/*.txt", "/foo/bar/baz.txt", opts3) == 0);
  expect(match("/foo/**/*.txt", "/foo/bar/baz/qux.txt", opts3) == 0);
  expect(match("/foo/**/bar.txt", "/foo/bar.txt", opts3) == 0);
  expect(match("/foo/**/**/bar.txt", "/foo/bar.txt", opts3) == 0);
  expect(match("/foo/**/*/baz.txt", "/foo/bar/baz.txt", opts3) == 0);
  expect(match("/foo/**/*.txt", "/foo/bar.txt", opts3) == 0);
  expect(match("/foo/**/**/*.txt", "/foo/bar.txt", opts3) == 0);
  expect(match("/foo/**/*/*.txt", "/foo/bar/baz.txt", opts3) == 0);
  expect(match("**/*.txt", "/foo/bar/baz/qux.txt", opts3) == 0);
  expect(match("**/foo.txt", "foo.txt", opts3) == 0);
  expect(match("**/*.txt", "foo.txt", opts3) == 0);
  expect(match("/foo/*", "/foo/bar/baz.txt", opts3) == 1);
  expect(match("/foo/*.txt", "/foo/bar/baz.txt", opts3) == 1);
  expect(match("/foo/*/*.txt", "/foo/bar/baz/qux.txt", opts3) == 1);
  expect(match("/foo/*/bar.txt", "/foo/bar.txt", opts3) == 1);
  expect(match("/foo/*/*/baz.txt", "/foo/bar/baz.txt", opts3) == 1);
  expect(match("/foo/**.txt", "/foo/bar/baz/qux.txt", opts3) == 1);
  expect(match("/foo/bar**/*.txt", "/foo/bar/baz/qux.txt", opts3) == 1);
  expect(match("/foo/bar**", "/foo/bar/baz.txt", opts3) == 1);
  expect(match("**/.txt", "/foo/bar/baz/qux.txt", opts3) == 1);
  expect(match("*/*.txt", "/foo/bar/baz/qux.txt", opts3) == 1);
  expect(match("*/*.txt", "foo.txt", opts3) == 1);

  toyo::path::globrex::globrex_options opts4;
  opts4.extended = true;
  opts4.globstar = true;
  expect(match("http://foo.com/*", "http://foo.com/bar/baz/jquery.min.js", opts4) == 1);
  expect(match("http://foo.com/*", "http://foo.com/bar/baz/jquery.min.js", opts3) == 1);

  toyo::path::globrex::globrex_options opts5;
  opts5.globstar = false;
  expect(match("http://foo.com/*", "http://foo.com/bar/baz/jquery.min.js", opts5) == 0);
  expect(match("http://foo.com/**", "http://foo.com/bar/baz/jquery.min.js", opts3) == 0);
  expect(match("http://foo.com/*/*/jquery.min.js", "http://foo.com/bar/baz/jquery.min.js", opts3) == 0);
  expect(match("http://foo.com/**/jquery.min.js", "http://foo.com/bar/baz/jquery.min.js", opts3) == 0);
  expect(match("http://foo.com/*/*/jquery.min.js", "http://foo.com/bar/baz/jquery.min.js", opts5) == 0);
  expect(match("http://foo.com/*/jquery.min.js", "http://foo.com/bar/baz/jquery.min.js", opts5) == 0);
  expect(match("http://foo.com/*/jquery.min.js", "http://foo.com/bar/baz/jquery.min.js", opts3) == 1);

  expect(match("(foo).txt", "(foo).txt", opts2) == 0);
  expect(match("?(foo).txt", "foo.txt", opts2) == 0);
  expect(match("?(foo).txt", ".txt", opts2) == 0);
  expect(match("?(foo|bar)baz.txt", "foobaz.txt", opts2) == 0);
  expect(match("?(ba[zr]|qux)baz.txt", "bazbaz.txt", opts2) == 0);
  expect(match("?(ba[zr]|qux)baz.txt", "barbaz.txt", opts2) == 0);
  expect(match("?(ba[zr]|qux)baz.txt", "quxbaz.txt", opts2) == 0);
  expect(match("?(ba[!zr]|qux)baz.txt", "batbaz.txt", opts2) == 0);
  expect(match("?(ba*|qux)baz.txt", "batbaz.txt", opts2) == 0);
  expect(match("?(ba*|qux)baz.txt", "batttbaz.txt", opts2) == 0);
  expect(match("?(ba*|qux)baz.txt", "quxbaz.txt", opts2) == 0);
  expect(match("?(ba?(z|r)|qux)baz.txt", "bazbaz.txt", opts2) == 0);
  expect(match("?(ba?(z|?(r))|qux)baz.txt", "bazbaz.txt", opts2) == 0);
  toyo::path::globrex::globrex_options opts6;
  opts6.extended = false;
  expect(match("?(foo).txt", "foo.txt", opts6) == 1);
  expect(match("?(foo|bar)baz.txt", "foobarbaz.txt", opts2) == 1);
  expect(match("?(ba[zr]|qux)baz.txt", "bazquxbaz.txt", opts2) == 1);
  expect(match("?(ba[!zr]|qux)baz.txt", "bazbaz.txt", opts2) == 1);

  expect(match("*(foo).txt", "foo.txt", opts2) == 0);
  expect(match("*foo.txt", "bofoo.txt", opts2) == 0);
  expect(match("*(foo).txt", "foofoo.txt", opts2) == 0);
  expect(match("*(foo).txt", ".txt", opts2) == 0);
  expect(match("*(fooo).txt", ".txt", opts2) == 0);
  expect(match("*(fooo).txt", "foo.txt", opts2) == 1);
  expect(match("*(foo|bar).txt", "foobar.txt", opts2) == 0);
  expect(match("*(foo|bar).txt", "barbar.txt", opts2) == 0);
  expect(match("*(foo|bar).txt", "barfoobar.txt", opts2) == 0);
  expect(match("*(foo|bar).txt", ".txt", opts2) == 0);
  expect(match("*(foo|ba[rt]).txt", "bat.txt", opts2) == 0);
  expect(match("*(foo|b*[rt]).txt", "blat.txt", opts2) == 0);
  expect(match("*(foo|b*[rt]).txt", "tlat.txt", opts2) == 1);
  expect(match("*(*).txt", "whatever.txt", opts4) == 0);
  expect(match("*(foo|bar)/**/*.txt", "foo/hello/world/bar.txt", opts4) == 0);
  expect(match("*(foo|bar)/**/*.txt", "foo/world/bar.txt", opts4) == 0);

  expect(match("+(foo).txt", "foo.txt", opts2) == 0);
  expect(match("+foo.txt", "+foo.txt", opts2) == 0);
  expect(match("+(foo).txt", ".txt", opts2) == 1);
  expect(match("+(foo|bar).txt", "foobar.txt", opts2) == 0);

  expect(match("@(foo).txt", "foo.txt", opts2) == 0);
  expect(match("@foo.txt", "@foo.txt", opts2) == 0);
  expect(match("@(foo|baz)bar.txt", "foobar.txt", opts2) == 0);
  expect(match("@(foo|baz)bar.txt", "foobazbar.txt", opts2) == 1);
  expect(match("@(foo|baz)bar.txt", "foofoobar.txt", opts2) == 1);
  expect(match("@(foo|baz)bar.txt", "toofoobar.txt", opts2) == 1);

  expect(match("!(boo).txt", "foo.txt", opts2) == 0);
  expect(match("!(foo|baz)bar.txt", "buzbar.txt", opts2) == 0);
  expect(match("!bar.txt", "!bar.txt", opts2) == 0);
  expect(match("!({foo,bar})baz.txt", "notbaz.txt", opts2) == 0);
  expect(match("!({foo,bar})baz.txt", "foobaz.txt", opts2) == 1);

  expect(match("foo//bar.txt", "foo/bar.txt") == 0);
  expect(match("foo///bar.txt", "foo/bar.txt") == 0);
  toyo::path::globrex::globrex_options opts7;
  opts7.strict = true;
  expect(match("foo///bar.txt", "foo/bar.txt", opts7) == 1);

  expect(match("**/*/?yfile.{md,js,txt}", "foo/bar/baz/myfile.md", opts2) == 0);
  expect(match("**/*/?yfile.{md,js,txt}", "foo/baz/myfile.md", opts2) == 0);
  expect(match("**/*/?yfile.{md,js,txt}", "foo/baz/tyfile.js", opts2) == 0);
  expect(match("[[:digit:]_.]/file.js", "1/file.js", opts2) == 0);
  expect(match("[[:digit:]_.]/file.js", "2/file.js", opts2) == 0);
  expect(match("[[:digit:]_.]/file.js", "_/file.js", opts2) == 0);
  expect(match("[[:digit:]_.]/file.js", "./file.js", opts2) == 0);
  expect(match("[[:digit:]_.]/file.js", "z/file.js", opts2) == 1);

  std::string testGlob = "**/{*.node,*.exe}";
  expect(toyo::path::globrex::match("C:\\Projects\\a\\b\\c\\addon.node", testGlob));
  expect(toyo::path::globrex::match("C:\\Projects\\a\\b\\c\\app.exe", testGlob));
  expect(!toyo::path::globrex::match("C:\\Projects\\a\\b\\c\\index.js", testGlob));
  expect(!toyo::path::globrex::match("C:\\Projects\\a\\b\\c\\favicon.ico", testGlob));
  return 0;
}
