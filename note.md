###  SQLite architecture

front-end:
- tokenizer
- parser
- code generator

back-end:
- virtual machine
- B-tree
- pager
- os interface

---

a simple schema:

column 	        type 

id	            integer 

username 	    varchar(32) 

email 	        varchar(255) 

`insert 1 jack aaa@abc.com`


### 安装和配置RSpec测试套件
要安装和配置 RSpec 测试套件，您需要按照以下步骤进行操作：

1. 首先，确保项目中已经安装了 Ruby 和 RubyGems。您可以通过在终端中运行 `ruby -v` 和 `gem -v` 命令来验证它们是否已经安装。

2. 在项目的根目录下，创建一个 `Gemfile` 文件，如果尚未创建。可以使用文本编辑器打开一个新文件，并将以下内容复制粘贴到文件中：

   ```ruby
   source 'https://rubygems.org'
   gem 'rspec'
   ```

   这将指定 RubyGems 的源和要安装的 RSpec gem。

3. 保存 `Gemfile` 文件，并在终端中导航到项目的根目录。

4. 运行以下命令来安装 RSpec 及其依赖项：

   ```shell
   bundle install
   ```

   这将使用 `bundle` 命令根据 `Gemfile` 文件中指定的依赖关系，安装所需的 RubyGems 包。

5. 安装完成后，可以使用以下命令初始化 RSpec：

   ```shell
   bundle exec rspec --init
   ```

   这将在项目中创建一个 `spec` 目录和一个 `spec_helper.rb` 文件，用于存放测试文件和配置。

6. 现在，可以在 `spec` 目录中编写您的测试文件。通常，RSpec 测试文件的命名约定是以 `_spec.rb` 结尾，例如 `my_spec.rb`。

7. 运行以下命令来执行您的 RSpec 测试套件：

   ```shell
   bundle exec rspec
   ```

   这将运行 `spec` 目录中的所有测试文件，并显示测试结果和概要。

