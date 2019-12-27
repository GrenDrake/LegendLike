#ifndef CONFIG_H
#define CONFIG_H

#include <map>
#include <string>

class Config {
public:
    bool loadFromFile(const std::string &file);
    bool writeToFile() const;
    const std::string& getString(const std::string &key, const std::string &defValue) const;
    int getInt(const std::string &key, int defValue) const;
    bool getBool(const std::string &key, bool defValue) const;
    void set(const std::string &key, const std::string &value);
    void set(const std::string &key, int value);
private:
    std::map<std::string, std::string> mKeys;
};


#endif
