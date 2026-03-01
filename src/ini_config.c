/* All we do here now is turn on logging or keep it off. 
   AppID and steam_appid.txt should be automatically set and configured in the code.
   However, if needed, just set them in the config file. 
   In fact, none of the configs are needed, not even the ini file itself.
   
   ~veeanti<3 */

#include "ini_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct KeyValue {
    char* key;
    char* value;
    struct KeyValue* next;
} KeyValue;

typedef struct Section {
    char* name;
    KeyValue* head;
    struct Section* next;
} Section;

struct IniConfig {
    char iniFilePath[260];
    Section* head;
};

static char* trim(char* str) {
    while (isspace(*str)) str++;
    if (*str == 0) return str;
    
    char* end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) end--;
    *(end + 1) = '\0';
    
    return str;
}

static void freeSection(Section* section) {
    if (!section) return;
    
    KeyValue* kv = section->head;
    while (kv) {
        KeyValue* temp = kv;
        kv = kv->next;
        free(temp->key);
        free(temp->value);
        free(temp);
    }
    
    free(section->name);
    free(section);
}

static Section* findSection(IniConfig* config, const char* sectionName) {
    Section* section = config->head;
    while (section) {
        if (strcmp(section->name, sectionName) == 0) {
            return section;
        }
        section = section->next;
    }
    return NULL;
}

static KeyValue* findKeyValue(Section* section, const char* key) {
    if (!section) return NULL;
    
    KeyValue* kv = section->head;
    while (kv) {
        if (strcmp(kv->key, key) == 0) {
            return kv;
        }
        kv = kv->next;
    }
    return NULL;
}

static Section* addSection(IniConfig* config, const char* sectionName) {
    Section* section = (Section*)malloc(sizeof(Section));
    if (!section) return NULL;
    
    section->name = _strdup(sectionName);
    section->head = NULL;
    section->next = config->head;
    config->head = section;
    
    return section;
}

static KeyValue* addKeyValue(Section* section, const char* key, const char* value) {
    KeyValue* kv = (KeyValue*)malloc(sizeof(KeyValue));
    if (!kv) return NULL;
    
    kv->key = _strdup(key);
    kv->value = _strdup(value);
    kv->next = section->head;
    section->head = kv;
    
    return kv;
}

static void createDefaultConfig(IniConfig* config) {
    IniConfig_SetValue(config, "Logging", "EnableLogging", "false");
    IniConfig_SetValue(config, "Logging", "LogFile", "uc_online.log");
    
    FILE* file = fopen(config->iniFilePath, "w");
    if (file) {
        fprintf(file, "[Logging]\n");
        fprintf(file, "# Turns on logging. Not much gets logged, so it's not exactly useful. I recommend keeping it set to false, however with it being rewritten, it seems to behave differently.\n");
        fprintf(file, "# It doesn't give you a chance to see what the issue was if you set something incorrectly, it just closes immediately or runs for a second and then closes. \n");
        fprintf(file, "# If you need it, set it to true. Otherwise, there's nothing really worth logging.\n");
        fprintf(file, "EnableLogging = false\n");
        fprintf(file, "LogFile = uc_online.log\n");
        fclose(file);
    }
}

IniConfig* IniConfig_Create(const char* iniFilePath) {
    if (!iniFilePath) return NULL;
    
    IniConfig* config = (IniConfig*)malloc(sizeof(IniConfig));
    if (!config) return NULL;
    
    strncpy(config->iniFilePath, iniFilePath, sizeof(config->iniFilePath) - 1);
    config->iniFilePath[sizeof(config->iniFilePath) - 1] = '\0';
    config->head = NULL;
    
    IniConfig_Load(config);
    
    return config;
}

void IniConfig_Destroy(IniConfig* config) {
    if (!config) return;
    
    Section* section = config->head;
    while (section) {
        Section* temp = section;
        section = section->next;
        freeSection(temp);
    }
    
    free(config);
}

void IniConfig_Load(IniConfig* config) {
    if (!config) return;
    
    Section* currentSection = NULL;
    char buffer[512];
    FILE* file = fopen(config->iniFilePath, "r");
    
    if (!file) {
        createDefaultConfig(config);
        return;
    }
    
    while (fgets(buffer, sizeof(buffer), file)) {
        char* line = trim(buffer);
        
        if (*line == '\0' || *line == ';' || *line == '#') {
            continue;
        }
        
        if (*line == '[') {
            char* end = strchr(line, ']');
            if (end) {
                *end = '\0';
                const char* sectionName = line + 1;
                
                currentSection = findSection(config, sectionName);
                if (!currentSection) {
                    currentSection = addSection(config, sectionName);
                }
            }
        } else if (currentSection) {
            char* equals = strchr(line, '=');
            if (equals) {
                *equals = '\0';
                
                char* key = trim(line);
                char* value = trim(equals + 1);
                
                KeyValue* existing = findKeyValue(currentSection, key);
                if (existing) {
                    free(existing->value);
                    existing->value = _strdup(value);
                } else {
                    addKeyValue(currentSection, key, value);
                }
            }
        }
    }
    
    fclose(file);
    
    if (!findSection(config, "Logging")) {
        addSection(config, "Logging");
    }
    if (!IniConfig_GetValue(config, "Logging", "EnableLogging", NULL)) {
        IniConfig_SetValue(config, "Logging", "EnableLogging", "false");
    }
    if (!IniConfig_GetValue(config, "Logging", "LogFile", NULL)) {
        IniConfig_SetValue(config, "Logging", "LogFile", "uc_online.log");
    }
}

void IniConfig_Save(IniConfig* config) {
    if (!config) return;
    
    FILE* file = fopen(config->iniFilePath, "w");
    if (!file) return;
    
    Section* section = config->head;
    while (section) {
        fprintf(file, "[%s]\n", section->name);
        
        KeyValue* kv = section->head;
        while (kv) {
            fprintf(file, "%s = %s\n", kv->key, kv->value);
            kv = kv->next;
        }
        
        fprintf(file, "\n");
        section = section->next;
    }
    
    fclose(file);
}

const char* IniConfig_GetValue(IniConfig* config, const char* section, const char* key, const char* defaultValue) {
    if (!config || !section || !key) {
        return defaultValue;
    }
    
    Section* sec = findSection(config, section);
    if (!sec) return defaultValue;
    
    KeyValue* kv = findKeyValue(sec, key);
    return kv ? kv->value : defaultValue;
}

void IniConfig_SetValue(IniConfig* config, const char* section, const char* key, const char* value) {
    if (!config || !section || !key || !value) return;
    
    Section* sec = findSection(config, section);
    if (!sec) {
        sec = addSection(config, section);
    }
    
    KeyValue* kv = findKeyValue(sec, key);
    if (kv) {
        free(kv->value);
        kv->value = _strdup(value);
    } else {
        addKeyValue(sec, key, value);
    }
}


bool IniConfig_GetEnableLogging(IniConfig* config) {
    const char* value = IniConfig_GetValue(config, "Logging", "EnableLogging", "false");
    if (_stricmp(value, "true") == 0 || strcmp(value, "1") == 0) {
        return true;
    }
    return false;
}

void IniConfig_SetEnableLogging(IniConfig* config, bool enable) {
    IniConfig_SetValue(config, "Logging", "EnableLogging", enable ? "true" : "false");
}

const char* IniConfig_GetLogFile(IniConfig* config) {
    return IniConfig_GetValue(config, "Logging", "LogFile", "uc_online.log");
}

void IniConfig_SetLogFile(IniConfig* config, const char* logFile) {
    IniConfig_SetValue(config, "Logging", "LogFile", logFile);
}
