Summary: Renders text in PNG format to be used as a watermark.
Name: watermark
Version: 0.1
Release: 1%{?dist}
License: MIT
Group: Applications/MultiMedia
Source: http://www.rorvick.com/software/watermark/watermark-0.1.tar.gz
URL: https://github.com/crorvick/watermark

%description
Reads text from stdin and renders to a PNG image of the specified
geometry.  The font size can be specified per line, either explicitly or
by specifying a target width.  Finally, the text is centered both
vertically and horizontally.  The final image is intended to be used
with ImageMagick's composite(1) to produce a watermark.

%prep
%setup

%build
%configure
make

%install
make install DESTDIR=%{buildroot}

%files
/usr/bin/watermark
%doc /usr/share/man/man1/watermark.1.gz
