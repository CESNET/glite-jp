Summary:Change me !!!
Name:glite-jp-ws-interface
Version:1.1.0
Release:0_U200510151557
Copyright:Open Source EGEE License
Vendor:EU EGEE project
Group:System/Application
Prefix:/opt/glite
BuildArch:i386
BuildRoot:%{_builddir}/%{name}-%{version}
Source:glite-jp-ws-interface-1.1.0_bin.tar.gz

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
%{prefix}/interface/JobProvenanceIS.wsdl
%{prefix}/interface/JobProvenanceTypes.wsdl
%{prefix}/share/doc/glite-jp-ws-interface-1.1.0/LICENSE
%{prefix}/share/doc/glite-jp-ws-interface-1.1.0/JobProvenance.html

%changelog

