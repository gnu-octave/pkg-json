% json_benchmark (tmp_dir)
%
%   `tmp_dir` is a writable directory, must be cleaned up manually.
%
% Returned is a Mx4 cell array, where each row contains:
%
%    test case name, url, time jsondecode, time jsonencode
%

% Copyright (C) 2021 The Octave Project Developers

% This file is intentionally Matlab compatible.

function result = json_benchmark (tmp_dir)

  if (nargin < 1)
    error ('json_benchmark: no temporary directory given.')
  end

  result = { ...
    'citm_catalog.json', ...
    'https://github.com/RichardHightower/json-parsers-benchmark/raw/master/data/citm_catalog.json'; ...
    'canada.json', ...
    'https://github.com/mloskot/json_benchmark/raw/master/data/canada.json'; ...
    'large-file.json', ...
    'https://github.com/json-iterator/test-data/raw/master/large-file.json'; ...
    'testdata_small.json', ...
    'https://savannah.gnu.org/bugs/download.php?file_id=51450'; ...
    'testdata_medium.json.zip', ...
    'https://savannah.gnu.org/bugs/download.php?file_id=51471'};

  old_dir = cd (tmp_dir);
  
  for i = 1:size (result, 1)
    if (exist (result{i,1}, 'file') ~= 2)
      urlwrite (result{i,2}, result{i,1});
    end
    [~, fname, ext] = fileparts (result{i,1});
    if (strcmp (ext, '.zip'))
      unzip (result{i,1});
      result{i,1} = fname;
    end
  end

  cd (old_dir);

  for i = 1:size (result, 1)
    fprintf ('%20s ', result{i,1})
    json_str = fileread (fullfile (tmp_dir, result{i,1}));
    tic ();
      octave_obj = jsondecode (json_str);
    result{i,3} = toc ();
    fprintf (' jsondecode: %f ', result{i,3});
    tic ();
      json_str2 = jsonencode (octave_obj);
    result{i,4} = toc ();
    fprintf (' jsonencode: %f\n', result{i,4});
  end

end
