SIMC_STANDALONE = false
EVDS_STANDALONE = false
RDRS_STANDALONE = false
solution "foxworks_editor"
   debugdir "../debug"
   dofile("./../external/simc/support/premake4_common.lua")
   dofile("./../external/simc/support/premake4.lua")
   dofile("./../external/evds/support/premake4.lua")
   dofile("./../external/rdrs/support/premake4.lua")
   
-- Create working directory
if not os.isdir("../debug") then os.mkdir("../debug") end




--------------------------------------------------------------------------------
-- Qt-related code and commands
--------------------------------------------------------------------------------
newoption {
   trigger     = "qtpath",
   description = "Path to Qt (used for foxworks_editor)",
   value       = "PATH"
}

newaction {
   trigger     = "moc",
   description = "Generate MOC files for Qt",
}

-- Default Qt paths
local QtDefault = "C:/Qt/4.8.5"
local QtMOCPath = "/bin/moc.exe"
local QtRCCPath = "/bin/rcc.exe"
local QtGenPath = "../qtmoc"

-- Set new default paths for Linux
if os.get() ~= "windows" then
  QtDefault = "/usr/share/qt4"
  QtMOCPath = "/bin/moc"
  QtRCCPath = "/bin/rcc"
end

-- Set possible Qt path
local QtPath = _OPTIONS["qtpath"] or QtDefault

-- Check if Qt is installed (assume installed under windows)
if not os.isfile(QtPath..QtMOCPath) then
  print("Qt not found at paths:")
  print("",QtDefault)
  if _OPTIONS["qtpath"] then print("",_OPTIONS["qtpath"]) end
  QtPath = nil
  return
end

-- Generate MOC files
GENERATED_MOCs = false
function generate_moc(files,do_warn,extrapath)
  local moc_files = os.matchfiles(files)
  for _,file in pairs(moc_files) do
    local file_ext = path.getextension(file)
    local out_file = QtGenPath..(extrapath or "").."/moc_"..path.getbasename(file)..".cpp"
    if file_ext == ".cpp" then
      out_file = QtGenPath..(extrapath or "").."/"..path.getbasename(file)..".moc"
    end

    local warnings = " -nw "
    if do_warn then warnings = " " end

    -- Get last modify date
    local mname = path.getdirectory(out_file).."/"..path.getbasename(out_file)..".tmp"
    local mtime = 0
    local mtest = io.open(mname,"r")
    if mtest then
      mtime = tonumber(mtest:read("*a"))
    end

    -- If applies, generate new file
    if mtime ~= os.stat(file).mtime then
      if GENERATED_MOCs == false then -- Only show message when generating
        print("Building MOC's...")
        GENERATED_MOCs = true
      end

      os.execute(QtPath..QtMOCPath.." "..file..warnings.."-o "..out_file)
    end

    -- Write last modify date
    mtest = io.open(mname,"w+")
    if mtest then
      mtest:write(os.stat(file).mtime)
    end
  end
end

-- Generate RCC files
GENERATED_RCCs = false
function generate_rcc(files,extrapath)
  local rcc_files = os.matchfiles(files)
  for _,file in pairs(rcc_files) do
    local file_ext = path.getextension(file)
    local out_file = QtGenPath..(extrapath or "").."/rcc_"..path.getbasename(file)..".cpp"

    -- Get last modify date
    local mname = path.getdirectory(out_file)..(extrapath or "").."/"..path.getbasename(out_file)..".tmp"
    local mtime = 0
    local mtest = io.open(mname,"r")
    if mtest then
      mtime = mtest:read("*n")
    end

    -- If applies, generate new file
    if mtime ~= os.stat(file).mtime then
      if GENERATED_RCCs == false then -- Only show message when generating
        print("Building RCC's...")
        GENERATED_RCCs = true
      end

      os.execute(QtPath..QtRCCPath.." -name "..path.getbasename(file).." -o "..out_file.." "..file)
    end

    -- Write last modify date
    mtest = io.open(mname,"w+")
    if mtest then
      mtest:write(os.stat(file).mtime)
    end
  end
end

-- Generate MOC's for Qt
if QtPath and (_ACTION ~= "clean") then
  if not os.isdir(QtGenPath) then
    os.mkdir(QtGenPath)
    os.mkdir(QtGenPath.."/glc")
  end

  -- Default libraries
  generate_moc("../external/qt-thumbwheel/**.h")
  generate_moc("../external/qt-solutions/qtpropertybrowser/src/**.h")
  generate_moc("../external/qt-solutions/qtpropertybrowser/src/**.cpp")
  generate_moc("../external/GLC_lib/src/**.h",false,"/glc")
  generate_rcc("../external/GLC_lib/src/*.qrc","/glc")

  -- Foxworks editor
  generate_moc("../source/**.h",true)
  generate_rcc("../resources/**.qrc")
end

if _ACTION == "moc" then return end


--------------------------------------------------------------------------------
-- FoxWorks Model Editor
--------------------------------------------------------------------------------
project "GLC_lib"
   uuid "55212504-025F-2D4A-81D3-5355DB38DCF5"
   kind "StaticLib"
   language "C++"
   
   includedirs {
     "../external/GLC_lib/src",
     "../external/GLC_lib/src/3rdparty/zlib",
     "../external/GLC_lib/src/3rdparty/glext",
     "../external/GLC_lib/src/3rdparty/quazip",
     "../external/GLC_lib/src/3rdparty/lib3ds",
     "../external/GLC_lib/src/3DWidget",
     "../external/GLC_lib/src/geometry",
     "../external/GLC_lib/src/glu",
     "../external/GLC_lib/src/io",
     "../external/GLC_lib/src/maths",
     "../external/GLC_lib/src/sceneGraph",
     "../external/GLC_lib/src/shading",
     "../external/GLC_lib/src/viewport",
   }
   files {
     "../external/GLC_lib/src/3rdparty/zlib/*.c",
     "../external/GLC_lib/src/3rdparty/zlib/*.h",
     "../external/GLC_lib/src/3rdparty/quazip/*.c",
     "../external/GLC_lib/src/3rdparty/quazip/*.cpp",
     "../external/GLC_lib/src/3rdparty/quazip/*.h",
     "../external/GLC_lib/src/3rdparty/lib3ds/*.c",
     "../external/GLC_lib/src/3rdparty/lib3ds/*.h",
     "../external/GLC_lib/src/*.cpp",
     "../external/GLC_lib/src/*.h",
     "../external/GLC_lib/src/3DWidget/*",
     "../external/GLC_lib/src/geometry/*",
     "../external/GLC_lib/src/glu/*",
     "../external/GLC_lib/src/io/*",
     "../external/GLC_lib/src/maths/*",
     "../external/GLC_lib/src/sceneGraph/*",
     "../external/GLC_lib/src/shading/*",
     "../external/GLC_lib/src/viewport/*",
   }
   excludes {
     "../external/GLC_lib/src/3rdparty/zlib/example.c"
   }
   defines { "GLC_LIB_STATIC", "LIB3DS_EXPORTS", "NO_vsnprintf" } --"CREATE_GLC_LIB_DLL",


   -- Additional Qt stuff
   includedirs {
     QtPath.."/include",
     QtPath.."/include/QtCore",
     QtPath.."/include/QtGui",
     QtPath.."/include/QtUiTools",
     QtPath.."/include/QtOpenGL",
     QtGenPath.."/glc"
   }
   files {
     QtGenPath.."/glc/**.cpp",
     QtGenPath.."/glc/**.moc",
   }
   



project "foxworks_editor"
   uuid "C84AD4D2-2D63-1842-871E-30B7C71BEA58"
   kind "ConsoleApp"
   language "C++"

   includedirs {
     "../external/simc/include",
     "../external/evds/include",
     "../external/rdrs/include",
     "../external/evds/addons",
--     "../external/nrlmsise-00",
     "../external/qt-solutions/qtpropertybrowser/src",
     "../external/qt-thumbwheel",
     "../external/GLC_lib/src",
     
     "../source",
     "../source/dialogs",
     "../source/editors",
     "../source/editors/evds",
     "../source/editors/ivss",
     "../source/editors/schematics",
     "../source/prop_sheet",
     "../source/util",
   }
   files {
     "../source/**.cpp",
     "../source/**.h",
     "../source/**.qrc",
     "../external/qt-solutions/qtpropertybrowser/src/**",
     "../external/qt-thumbwheel/**",
     "../external/evds/addons/evds_antenna.c",
     "../external/evds/addons/evds_antenna.h",
     "../external/evds/addons/evds_train_wheels.c",
     "../external/evds/addons/evds_train_wheels.h",
--     "../external/evds/addons/evds_nrlmsise-00.c",
--     "../external/evds/addons/evds_nrlmsise-00.h",
--     "../external/nrlmsise-00/nrlmsise-00.c",
--     "../external/nrlmsise-00/nrlmsise-00_data.c" }
   }
   links { "rdrs","evds","simc","GLC_lib" }
   defines { "GLC_LIB_STATIC" }
   
   
   -- Additional Qt stuff
   includedirs {
     QtPath.."/include",
     QtPath.."/include/QtCore",
     QtPath.."/include/QtGui",
     QtPath.."/include/QtUiTools",
     QtPath.."/include/QtOpenGL",
     QtGenPath
   }
   libdirs { QtPath.."/lib" }
   files {
     QtGenPath.."/rcc_resources.cpp",
     QtGenPath.."/moc_fw*.cpp",
     QtGenPath.."/moc_qtthumbwheel.cpp",
     QtGenPath.."/moc_qtpropertybrowserutils_p.cpp"
   }
   
   configuration { "windows", "Debug*" }
      links { "QtCored4", "QtGuid4", "QtUiToolsd", "QtOpenGLd4", "opengl32" }
   configuration { "not windows", "Debug*" }
      links { "QtCored", "QtGuid", "QtUiToolsd", "QtOpenGLd" }
   configuration { "windows", "Release*" }
      links { "QtCore4", "QtGui4", "QtUiTools", "QtOpenGL4", "opengl32" }
   configuration { "not windows", "Release*" }
      links { "QtCore", "QtGui", "QtUiTools", "QtOpenGL" }
