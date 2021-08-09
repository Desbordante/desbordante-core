cd src/components/$1
mkdir __test__
cd __test__
touch $1.test.js
echo "import React from \"react\";
import ReactDOM from \"react-dom\";
import $1 from \"../$1\";

import \"../../../mocks\";

it(\"renders without crashing\", () => {
  const div = document.createElement(\"div\");
  ReactDOM.render(<$1 />, div);
});
" > $1.test.js
cd ../../