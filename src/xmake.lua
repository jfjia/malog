add_rules("mode.debug", "mode.release")

target("malog")
    set_kind("static")
    add_files("malog.cpp")
    set_warnings("all", "error")
    set_languages("c++11")
    add_includedirs(".", {public = true})
