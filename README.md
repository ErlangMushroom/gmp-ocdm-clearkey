# gmp-ocdm-clearkey
how to enable it on Mac & linux ?

1. put the folder 'gmp-ocdm-clearkey' in dom/media, and modify dom/media/moz.build
<pre>
  DIRS += [
     'gmp',
     'gmp-plugin',
     'gmp-plugin-openh264',
     'gmp-ocdm-clearkey', // add gmp-ocdm-clearkey to compilation
     'imagecapture',
     'mediasink',
     'mediasource',
</pre>

2. modify GMPProvider.jsm (toolkit/mozapps/extensions/internal/GMPProvider.jsm)
<pre>
   const CLEARKEY_PLUGIN_ID     = "gmp-ocdm-clearkey"; // changed from "gmp-clearkey"
   const CLEARKEY_VERSION       = "1.0";               // changed from "0.1"
</pre>

3. disable sandbox (modify content_process_main() in ipc/contentproc/plugin-container.cpp)
<pre>
     if (XRE_GetProcessType() == GeckoProcessType_GMPlugin) {
        loader = mozilla::gmp::CreateGMPLoader(nullptr); // disable starter
     }
</pre>

how to enable it on B2G ?

1. put the folder 'gmp-ocdm-clearkey' in dom/media, and modify dom/media/moz.build
<pre>
  DIRS += [
     'gmp',
     'gmp-plugin',
     'gmp-plugin-openh264',
     'gmp-ocdm-clearkey', // add gmp-ocdm-clearkey to compilation
     'imagecapture',
     'mediasink',
     'mediasource',
  ]
</pre>

2. modify configure.in to add include path for openssl
  append "-I$gonkdir/external/openssl/include" to GONK_INCLUDES variable

3. modify b2g/chrome/content/shell.js
<pre>
  try {
   let gmpService = Cc["@mozilla.org/gecko-media-plugin-service;1"]
                      .getService(Ci.mozIGeckoMediaPluginChromeService);
   gmpService.addPluginDirectory("/system/b2g/gmp-ocdm-clearkey/1.0"); // changed from "gmp-clearkey/0.1"
 } catch(e) {
   dump("Failed to add clearkey path! " + e + "\n");
 }
</pre>

4. modify b2g/installer/package-manifest.in
<pre>
 #ifdef MOZ_EME
 @RESPATH@/gmp-ocdm-clearkey/1.0/@DLL_PREFIX@ocdm-clearkey@DLL_SUFFIX@
 @RESPATH@/gmp-ocdm-clearkey/1.0/ocdm-clearkey.info
 @RESPATH@/@DLL_PREFIX@ocdmi@DLL_SUFFIX@
 #endif
</pre>
