Summary:Change me !!!
Name:glite-jp-ws-interface
Version:0.0.0
Release:0
Copyright:Open Source EGEE License
Vendor:EU EGEE project
Group:System/Application
Prefix:/opt/glite
BuildArch:x86_64
BuildRoot:%{_builddir}/%{name}-%{version}
Source:glite-jp-ws-interface-0.0.0_bin.tar.gz

%define debug_package %{nil}

%description
Change me !!!

%prep
 

%setup -c

%build
 

%install
 

%clean
 
%pre
%post
%preun
%postun
%files
%defattr(-,root,root)
%{prefix}/interface/JobProvenancePS.wsdl
%{prefix}/interface/JobProvenanceTypes.wsdl
%{prefix}/share/doc/glite-jp-ws-interface-0.0.0/LICENSE

%changelog

