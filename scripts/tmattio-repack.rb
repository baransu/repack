class TmattioRepack < Formula
  VERSION = '0.1.0'
  desc ''
  homepage 'https://github.com/baransu/repack'
  url "https://github.com/baransu/repack/releases/download/v#{VERSION}/repack-darwin-x64.zip"
  version VERSION

  bottle :unneeded

  test do
  end

  def install
    mv "repack.exe", "repack"
    bin.install 'repack'
  end
end
