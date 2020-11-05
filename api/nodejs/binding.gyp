{
  "targets": 
  [
    {
      "target_name": "engage-engine",
      "sources": [ "engage.cpp" ],
      "include_dirs": 
      [ 
        "<(module_root_dir)/include/",    
        "<!(node -e \"require('nan')\")",
        "<!(node -e \"require('node-gyp')\")",
      ],

      "link_settings": 
      {
        "libraries": ["-lengage-shared"],
        "library_dirs": ["<(module_root_dir)/lib/<!(node -p process.platform).<!(node -p process.arch)"]
      }
    }
  ]
}
