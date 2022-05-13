import os
from conans import ConanFile, MSBuild


class CicadaConan(ConanFile):
    license = "MIT"
    name = "Cicada-Player"
    settings = "os", "compiler", "build_type", "arch"
    generators = "visual_studio"

    def package(self):
    
        self.copy("*.h", dst="include/cicada", src="mediaPlayer", keep_path=True)
        
        self.copy("ffmpeg-4.dll", dst="bin", src="external/install/ffmpeg/win32/i686/bin", keep_path=False)

        if self.settings.build_type=="Debug":
            self.copy("media_player.lib", dst="lib", src="build32/cmdline/mediaPlayer.out/Debug", keep_path=False)
            self.copy("media_player.dll", dst="bin", src="build32/cmdline/mediaPlayer.out/Debug", keep_path=False)
            self.copy("media_player.pdb", dst="bin", src="build32/cmdline/mediaPlayer.out/Debug", keep_path=False)
        else:
            self.copy("media_player.lib", dst="lib", src="build32/cmdline/mediaPlayer.out/RelWithDebInfo", keep_path=False)
            self.copy("media_player.dll", dst="bin", src="build32/cmdline/mediaPlayer.out/RelWithDebInfo", keep_path=False)
            self.copy("media_player.pdb", dst="bin", src="build32/cmdline/mediaPlayer.out/RelWithDebInfo", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["media_player"]
