#include "userver/logging/log.hpp"
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <queue>
#include <set>


namespace dblpservice {

    class articlePart {
        public:
            std::string title;
            std::set<std::string> author;
            std::string journal;
            std::string year;
            void clear() {
                title.clear();
                author.clear();
                journal.clear();
                year.clear();
            }
            bool good() {
                if (title.empty() || author.empty() || journal.empty() || year.empty()) {
                    return false;
                }
                return true;
            }
    };

    class dblpXmlArticleParser {
        public:
            dblpXmlArticleParser(const std::set<std::string>& articleTypes) :
                articleTypes_(articleTypes)
            {}

        void write(const std::string& part) {
            tokenize(part);
            process();
            // tokens_.clear();
            // result_.clear();
        }

        std::vector<articlePart>&& get() {
            return std::move(result_);
        }

        private:

            std::string tokenizerBuffer_;
            void tokenize(const std::string& data) {
                for (char c : data) {
                    if (c == '\n') {
                        continue;
                    }
                    if (c == '<') {
                        tokens_.emplace_back(std::move(tokenizerBuffer_));
                    }
                    tokenizerBuffer_ += c;
                    if (c == '>') {
                        tokens_.emplace_back(std::move(tokenizerBuffer_));
                    }
                }
            }
            std::vector<std::string> tokens_;

            articlePart currentArticle;
            std::string currentTag;
            std::string currentArticleTag;
            void process() {
                for (const auto& token : tokens_) {
                    if (token.front() == '<') {
                        auto tag = getTag(token);
                        if (tag.front() == '/') {
                            // close
                            // LOG_ERROR() << tag << " closed " << currentArticleTag;
                            if (tag.substr(1) == currentArticleTag) {
                                // LOG_ERROR() << "PUSH " << currentArticle.author;
                                if (currentArticle.good()) {
                                    result_.push_back(currentArticle);
                                }
                                currentArticle.clear();
                            }
                            currentTag.clear();
                        } else {
                            // open
                            currentTag = tag;
                            if (articleTypes_.count(std::string(tag))) {
                                currentArticleTag = tag;
                            }
                        }
                    } else {
                        if (currentTag.empty())
                            continue;
                        if (currentTag == "title") {
                            currentArticle.title = token;
                        } else if (currentTag == "year") {
                            currentArticle.year = token;
                        } else if (currentTag == "journal") {
                            currentArticle.journal = token;
                        } else if (currentTag == "author") {
                            currentArticle.author.insert(token);
                        } else {
                            // skip tag
                        }
                    }
                }
                tokens_.clear();
            }

            std::vector<articlePart> result_;

            std::string_view getTag(const std::string& token) {
                auto pos = token.find(' ');
                if (pos==std::string::npos) {
                    return std::string_view(token).substr(1, token.size() - 2);
                } else {
                    return std::string_view(token).substr(1, pos - 1);
                }
            }

            std::set<std::string> articleTypes_;
    };


} // dblpservice