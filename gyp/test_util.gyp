{
    'targets': [
        {
            'target_name': 'test_util',
            'type': 'executable',
            'variables': {
                'src' : '../src/utils',
            },
            'include_dirs': [
                '../src/utils',
            ],
            'sources': [
              "../src/AppUtil.h",
              "../src/AppUtil.cpp",
              "<(src)/BaseUtil.h",
              "<(src)/BaseUtil.cpp",
              "<(src)/BitManip.h",
              "<(src)/ByteOrderDecoder.h",
              "<(src)/ByteOrderDecoder.cpp",
              "<(src)/CmdLineParser.h",
              "<(src)/CmdLineParser.cpp",
              "<(src)/CryptoUtil.h",
              "<(src)/CryptoUtil.cpp",
              "<(src)/CssParser.h",
              "<(src)/CssParser.cpp",
              "<(src)/Dict.h",
              "<(src)/Dict.cpp",
              "<(src)/DebugLog.h",
              "<(src)/DebugLog.cpp",
              "<(src)/FileUtil.h",
              "<(src)/FileUtil.cpp",
              "<(src)/GeomUtil.h",
              "<(src)/HtmlParserLookup.h",
              "<(src)/HtmlParserLookup.cpp",
              "<(src)/HtmlPrettyPrint.h",
              "<(src)/HtmlPrettyPrint.cpp",
              "<(src)/HtmlPullParser.h",
              "<(src)/HtmlPullParser.cpp",
              "<(src)/JsonParser.h",
              "<(src)/JsonParser.cpp",
              "<(src)/Scoped.h",
              "<(src)/SettingsUtil.h",
              "<(src)/SettingsUtil.cpp",
              "<(src)/SimpleLog.h",
              "<(src)/StrFormat.h",
              "<(src)/StrFormat.cpp",
              "<(src)/StrUtil.h",
              "<(src)/StrUtil.cpp",
              "<(src)/SquareTreeParser.h",
              "<(src)/SquareTreeParser.cpp",
              "<(src)/TrivialHtmlParser.h",
              "<(src)/TrivialHtmlParser.cpp",
              "<(src)/UtAssert.h",
              "<(src)/UtAssert.cpp",
              "<(src)/VarintGob.h",
              "<(src)/VarintGob.cpp",
              "<(src)/Vec.h",
              "<(src)/WinUtil.h",
              "<(src)/WinUtil.cpp",
              "<(src)/tests/BaseUtil_ut.cpp",
              "<(src)/tests/ByteOrderDecoder_ut.cpp",
              "<(src)/tests/CmdLineParser_ut.cpp",
              "<(src)/tests/CryptoUtil_ut.cpp",
              "<(src)/tests/CssParser_ut.cpp",
              "<(src)/tests/Dict_ut.cpp",
              "<(src)/tests/FileUtil_ut.cpp",
              "<(src)/tests/HtmlPrettyPrint_ut.cpp",
              "<(src)/tests/HtmlPullParser_ut.cpp",
              "<(src)/tests/JsonParser_ut.cpp",
              "<(src)/tests/SettingsUtil_ut.cpp",
              "<(src)/tests/SimpleLog_ut.cpp",
              "<(src)/tests/SquareTreeParser_ut.cpp",
              "<(src)/tests/StrFormat_ut.cpp",
              "<(src)/tests/StrUtil_ut.cpp",
              "<(src)/tests/TrivialHtmlParser_ut.cpp",
              "<(src)/tests/VarintGob_ut.cpp",
              "<(src)/tests/Vec_ut.cpp",
              "<(src)/tests/WinUtil_ut.cpp",
              "../src/UnitTests.cpp",
              "../src/mui/SvgPath.h",
              "../src/mui/SvgPath.cpp",
              "../src/mui/SvgPath_ut.cpp",
              "../tools/tests/UnitMain.cpp",
            ],
            'defines': [
                'NO_LIBMUPDF'
            ],
            'link_settings': {
                'libraries': [
                    'gdiplus.lib',
                    'comctl32.lib',
                    'shlwapi.lib',
                    'Version.lib',
                    'user32.lib',
                    'kernel32.lib',
                    'gdi32.lib',
                    'ole32.lib',
                    'advapi32.lib',
                    'shell32.lib',
                    'oleaut32.lib',
                    'winspool.lib',
                ]
            },
            'msvs_settings':
            {
              'VCLinkerTool':
              {
                'SubSystem': '1',   # Console
              },
            },
        },
    ],
}
