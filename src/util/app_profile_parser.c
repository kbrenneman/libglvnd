/*
 * Copyright (c) 2019, NVIDIA CORPORATION.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * unaltered in all copies or substantial portions of the Materials.
 * Any additions, deletions, or changes to the original source files
 * must be clearly indicated in accompanying documentation.
 *
 * If only executable code is distributed, then the accompanying
 * documentation must state that "this software is based in part on the
 * work of the Khronos Group."
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fnmatch.h>
#include <sys/stat.h>
#include <unistd.h>

#include "app_profile.h"
#include "glvnd_list.h"
#include "cJSON.h"
#include "utils_misc.h"

static const char *PROFILES_PATTERN = "*.profiles.json";
static const char *RULES_PATTERN = "*.rules.json";
static const char *USER_PROFILES_DIR = "glvnd/profiles.d";
static const char *PROFILES_DIRS[] =
{
    SYSCONFDIR_BASE "/app_profiles.d",
    DATADIR_BASE "/app_profiles.d",
    NULL
};
static const int PROFILE_DIR_COUNT = sizeof(PROFILES_DIRS) / sizeof(PROFILES_DIRS);

typedef struct
{
    char *name;
    char *data;
    GLboolean onlyInServerList;
    GLboolean disabled;

    struct glvnd_list entry;
} ParserProfileVendorEntry;

typedef struct
{
    char *name;
    GLboolean overrideRule;
    struct glvnd_list vendors;

    struct glvnd_list entry;
} ParserProfileRuleEntry;

typedef struct
{
    char execName[256];
    const char **configDirs;
    struct glvnd_list vendors;
    struct glvnd_list rules;
    GLboolean rulesLoaded;
    GLboolean overrideProfile;

    char *userConfigDirBuf;
    char **envProfileDirsBuf;
} ParserState;

// TODO: This is duplicated from EGL, so it should probably go into some
// common file.
static cJSON *ReadJSONFile(const char *filename)
{
    FILE *in = NULL;
    char *buf = NULL;
    cJSON *root = NULL;
    struct stat st;

    in = fopen(filename, "r");
    if (in == NULL) {
        goto done;
    }

    if (fstat(fileno(in), &st) != 0) {
        goto done;
    }

    buf = (char *) malloc(st.st_size + 1);
    if (buf == NULL) {
        goto done;
    }

    if (fread(buf, st.st_size, 1, in) != 1) {
        goto done;
    }
    buf[st.st_size] = '\0';

    root = cJSON_Parse(buf);

done:
    if (in != NULL) {
        fclose(in);
    }
    if (buf != NULL) {
        free(buf);
    }
    return root;
}

static void FreeVendorEntry(ParserProfileVendorEntry *vendor)
{
    if (vendor != NULL) {
        free(vendor->data);
        free(vendor);
    }
}

static void FreeRuleEntry(ParserProfileRuleEntry *rule)
{
    if (rule != NULL) {
        ParserProfileVendorEntry *vendor, *vendorTmp;
        glvnd_list_for_each_entry_safe(vendor, vendorTmp, &rule->vendors, entry) {
            glvnd_list_del(&vendor->entry);
            FreeVendorEntry(vendor);
        }
        free(rule);
    }
}

/**
 * Returns the name of the executable for the current process.
 *
 * TODO: Figure out what to do on systems that don't use procfs.
 */
static GLboolean GetExecutableName(char *buf, size_t maxLen)
{
    char path[256];
    ssize_t r;

    snprintf(path, sizeof(path), "/proc/%d/exe", (int) getpid());

    r = readlink(path, buf, maxLen - 1);
    if (r <= 0)
    {
        return GL_FALSE;
    }
    buf[r] = '\0';
    return GL_TRUE;
}

static GLboolean InitParserState(ParserState *parser)
{
    const char *env;
    memset(parser, 0, sizeof(ParserState));
    glvnd_list_init(&parser->vendors);
    glvnd_list_init(&parser->rules);

    if (!GetExecutableName(parser->execName, sizeof(parser->execName))) {
        return GL_FALSE;
    }

    // Figure out which directories we're going to scan for config files.
    env = getenv("__GLVND_PROFILE_DIRS");
    if (env != NULL) {
        parser->envProfileDirsBuf = SplitString(env, NULL, ":");
        if (parser->envProfileDirsBuf == NULL) {
            return GL_FALSE;
        }
        parser->configDirs = (const char **) parser->envProfileDirsBuf;
    } else {
        int i, index;

        parser->configDirs = malloc((PROFILE_DIR_COUNT + 2) * sizeof(const char *));
        if (parser->configDirs == NULL) {
            return GL_FALSE;
        }

        // Figure out where the user's profile directory is.
        env = getenv("XDG_CONFIG_HOME");
        if (env != NULL) {
            if (glvnd_asprintf(&parser->userConfigDirBuf, "%s/%s", env, USER_PROFILES_DIR) < 0) {
                return GL_FALSE;
            }
        }
        if (parser->userConfigDirBuf == NULL) {
            env = getenv("HOME");
            if (env != NULL) {
                if (glvnd_asprintf(&parser->userConfigDirBuf, "%s/.config/%s", env, USER_PROFILES_DIR) < 0) {
                    return GL_FALSE;
                }
            }
        }

        index = 0;
        if (parser->userConfigDirBuf != NULL) {
            parser->configDirs[index++] = parser->userConfigDirBuf;
        }
        for (i=0; i<PROFILE_DIR_COUNT; i++) {
            parser->configDirs[index++] = PROFILES_DIRS[i];
        }
        parser->configDirs[index++] = NULL;
    }
    return GL_TRUE;
}

static void FreeParserState(ParserState *parser) {
    ParserProfileVendorEntry *vendor, *vendorTmp;
    ParserProfileRuleEntry *rule, *ruleTmp;
    glvnd_list_for_each_entry_safe(vendor, vendorTmp, &parser->vendors, entry) {
        glvnd_list_del(&vendor->entry);
        FreeVendorEntry(vendor);
    }

    glvnd_list_for_each_entry_safe(rule, ruleTmp, &parser->rules, entry) {
        glvnd_list_del(&rule->entry);
        FreeRuleEntry(rule);
    }

    free(parser->configDirs);
    free(parser->userConfigDirBuf);
    free(parser->envProfileDirsBuf);
}

static ParserProfileRuleEntry *FindParserRule(ParserState *parser, const char *name, GLboolean add)
{
    ParserProfileRuleEntry *rule;

    glvnd_list_for_each_entry(rule, &parser->rules, entry) {
        if (strcmp(rule->name, name) == 0) {
            return rule;
        }
    }

    if (add) {
        size_t len = strlen(name) + 1;
        rule = malloc(sizeof(ParserProfileRuleEntry) + len);
        if (rule == NULL) {
            return NULL;
        }
        rule->name = (char *) (rule + 1);
        memcpy(rule->name, name, len);
        rule->overrideRule = GL_FALSE;
        glvnd_list_init(&rule->vendors);
        glvnd_list_append(&rule->entry, &parser->rules);
        return rule;
    } else {
        return NULL;
    }
}

static ParserProfileVendorEntry *FindParserVendor(struct glvnd_list *vendorList, const char *name)
{
    ParserProfileVendorEntry *vendor;
    glvnd_list_for_each_entry(vendor, vendorList, entry) {
        if (strcmp(vendor->name, name) == 0) {
            return vendor;
        }
    }
    return NULL;
}

static GLboolean ParseVendorNodeCommon(struct glvnd_list *vendorList, cJSON *vendorNode)
{
    const char *name;
    size_t len;
    cJSON *node;
    ParserProfileVendorEntry *vendor;

    if (!cJSON_IsObject(vendorNode)) {
        return GL_TRUE;
    }

    node = cJSON_GetObjectItem(vendorNode, "vendor_name");
    if (!cJSON_IsString(node)) {
        return GL_TRUE;
    }
    name = node->valuestring;

    if (FindParserVendor(vendorList, name) != NULL) {
        return GL_TRUE;
    }

    len = strlen(name) + 1;
    vendor = malloc(sizeof(ParserProfileVendorEntry) + len);
    vendor->name = (char *) (vendor + 1);
    vendor->data = NULL;
    vendor->disabled = GL_FALSE;
    vendor->onlyInServerList = GL_FALSE;
    memcpy(vendor->name, name, len);

    node = cJSON_GetObjectItem(vendorNode, "disable");
    if (cJSON_IsTrue(node)) {
        vendor->disabled = GL_TRUE;
    } else {
        node = cJSON_GetObjectItem(vendorNode, "vendor_data");
        if (node != NULL) {
            vendor->data = cJSON_PrintUnformatted(node);
            if (vendor->data == NULL) {
                free(vendor);
                return GL_FALSE;
            }
        }

        node = cJSON_GetObjectItem(vendorNode, "only_in_server_list");
        if (cJSON_IsTrue(node)) {
            vendor->onlyInServerList = GL_TRUE;
        }
    }

    glvnd_list_append(&vendor->entry, vendorList);
    return GL_TRUE;
}

static GLboolean ParseRuleNode(ParserState *parser, cJSON *ruleNode)
{
    const char *name;
    cJSON *node;
    ParserProfileRuleEntry *rule;

    if (!cJSON_IsObject(ruleNode)) {
        return GL_TRUE;
    }

    node = cJSON_GetObjectItem(ruleNode, "rule_name");
    if (!cJSON_IsString(node)) {
        return GL_TRUE;
    }
    name = node->valuestring;

    rule = FindParserRule(parser, name, GL_TRUE);
    if (rule == NULL) {
        return GL_FALSE;
    }
    if (rule->overrideRule) {
        return GL_TRUE;
    }

    node = cJSON_GetObjectItem(ruleNode, "override");
    if (cJSON_IsTrue(node)) {
        rule->overrideRule = GL_TRUE;
    }

    return ParseVendorNodeCommon(&rule->vendors, ruleNode);
}

static GLboolean CheckVersion(cJSON *root)
{
    if (!cJSON_IsObject(root)) {
        return GL_FALSE;
    }

    // TODO: Find and check a version number
    return GL_TRUE;
}

static GLboolean ParseRulesFile(ParserState *parser, const char *path)
{
    cJSON *root;
    cJSON *node;

    root = ReadJSONFile(path);
    if (root == NULL) {
        return GL_TRUE;
    }

    if (!CheckVersion(root)) {
        cJSON_Delete(root);
        return GL_TRUE;
    }

    node = cJSON_GetObjectItem(root, "rules");
    if (cJSON_IsArray(node)) {
        for (node = node->child; node != NULL; node = node->next) {
            if (!ParseRuleNode(parser, node)) {
                cJSON_Delete(root);
                return GL_FALSE;
            }
        }
    }
    cJSON_Delete(root);
    return GL_TRUE;
}

static int RulesScandirFilter(const struct dirent *ent)
{
    return (fnmatch(RULES_PATTERN, ent->d_name, 0) == 0);
}

static GLboolean ReadRulesDir(ParserState *parser, const char *dirname)
{
    char **files = ScandirArray(dirname, RulesScandirFilter);
    if (files != NULL) {
        int i;
        for (i=0; files[i] != NULL; i++) {
            if (!ParseRulesFile(parser, files[i])) {
                free(files);
                return GL_FALSE;
            }
        }
        free(files);
    }
    return GL_TRUE;
}

static GLboolean ReadRulesFiles(ParserState *parser)
{
    if (!parser->rulesLoaded) {
        int i;
        parser->rulesLoaded = GL_TRUE;
        for (i=0; parser->configDirs[i] != NULL; i++) {
            if (!ReadRulesDir(parser, parser->configDirs[i])) {
                return GL_FALSE;
            }
        }
    }

    return GL_TRUE;
}

static GLboolean ExecutableNameMatches(ParserState *parser, const char *suffix)
{
    if (suffix[0] == '/')
    {
        // If the profile's string starts with a '/', then it must match
        // exactly, not just a suffix.
        return strcmp(parser->execName, suffix) == 0;
    }
    else
    {
        size_t nameLen = strlen(parser->execName);
        size_t suffixLen = strlen(suffix);

        if (nameLen >= suffixLen)
        {
            size_t offset = nameLen - suffixLen;

            if (offset > 0)
            {
                // Only match complete path components. That is, if the config file
                // says "glxgears", then we'll match "glxgears" or
                // "/usr/bin/glxgears", but not "fooglxgears".
                if (parser->execName[offset - 1] != '/')
                {
                    return GL_FALSE;
                }
            }

            return strcmp(parser->execName + offset, suffix) == 0;
        }
    }

    return GL_FALSE;
}

static GLboolean ProfileNodeMatches(ParserState *parser, cJSON *profileNode)
{
    cJSON *node;

    if (!cJSON_IsObject(profileNode)) {
        return GL_FALSE;
    }

    node = cJSON_GetObjectItem(profileNode, "match");
    if (node != NULL) {
        if (cJSON_IsString(node)) {
            return ExecutableNameMatches(parser, node->valuestring);
        } else if (cJSON_IsArray(node)) {
            for (node = node->child; node != NULL; node = node->next) {
                if (cJSON_IsString(node)
                        && ExecutableNameMatches(parser, node->valuestring)) {
                    return GL_TRUE;
                }
            }
        }
    }

    return GL_FALSE;
}

static GLboolean ResolveRule(ParserState *parser, const char *ruleName)
{
    ParserProfileRuleEntry *rule;

    if (!ReadRulesFiles(parser)) {
        return GL_FALSE;
    }

    rule = FindParserRule(parser, ruleName, GL_FALSE);
    if (rule != NULL) {
        // Remove the rule after we resolve it. Any further references would
        // end up with the same vendor names, which would get thrown away.
        ParserProfileVendorEntry *vendor, *vendorTmp;
        glvnd_list_for_each_entry_safe(vendor, vendorTmp, &rule->vendors, entry) {
            glvnd_list_del(&vendor->entry);

            if (FindParserVendor(&parser->vendors, vendor->name) == NULL) {
                glvnd_list_append(&vendor->entry, &parser->vendors);
            } else {
                FreeVendorEntry(vendor);
            }
        }

        glvnd_list_del(&rule->entry);
        FreeRuleEntry(rule);
    }
    return GL_TRUE;
}

static GLboolean ParseProfileVendorNode(ParserState *parser, cJSON *vendorNode)
{
    cJSON *node;

    if (!cJSON_IsObject(vendorNode)) {
        return GL_TRUE;
    }

    node = cJSON_GetObjectItem(vendorNode, "rule_name");
    if (node != NULL) {
        if (!cJSON_IsString(node)) {
            return GL_TRUE;
        }
        return ResolveRule(parser, node->valuestring);
    } else {
        return ParseVendorNodeCommon(&parser->vendors, vendorNode);
    }
}

static GLboolean ParseProfileNode(ParserState *parser, cJSON *profileNode)
{
    cJSON *node;

    node = cJSON_GetObjectItem(profileNode, "vendors");
    if (cJSON_IsArray(node)) {
        for (node = node->child; node != NULL; node = node->next) {
            if (!ParseProfileVendorNode(parser, node)) {
                return GL_FALSE;
            }
        }
    }

    node = cJSON_GetObjectItem(profileNode, "override");
    if (cJSON_IsTrue(node)) {
        parser->overrideProfile = GL_TRUE;
    }

    return GL_TRUE;
}

static GLboolean ParseProfileFile(ParserState *parser, const char *path)
{
    cJSON *root;
    cJSON *node;
    GLboolean ret = GL_TRUE;

    root = ReadJSONFile(path);
    if (root == NULL) {
        return GL_TRUE;
    }

    if (!CheckVersion(root)) {
        cJSON_Delete(root);
        return GL_TRUE;
    }

    node = cJSON_GetObjectItem(root, "profiles");
    if (cJSON_IsArray(node)) {
        for (node = node->child; node != NULL; node = node->next) {
            if (ProfileNodeMatches(parser, node)) {
                ret = ParseProfileNode(parser, node);
                break;
            }
        }
    }
    cJSON_Delete(root);
    return ret;
}

static int ProfileScandirFilter(const struct dirent *ent)
{
    return (fnmatch(PROFILES_PATTERN, ent->d_name, 0) == 0);
}

static GLboolean ParseProfileDir(ParserState *parser, const char *dirname)
{
    char **files;
    int i;

    files = ScandirArray(dirname, ProfileScandirFilter);
    if (files != NULL) {
        for (i=0; files[i] != NULL && !parser->overrideProfile; i++) {
            if (!ParseProfileFile(parser, files[i])) {
                free(files);
                return GL_FALSE;
            }
        }
        free(files);
    }

    return GL_TRUE;
}

void glvndLoadProfileConfig(GLVNDappProfile *profile)
{
    ParserState parser;
    ParserProfileVendorEntry *vendor;
    GLboolean success = GL_TRUE;
    const char *envRuleName;

    if (!InitParserState(&parser)) {
        FreeParserState(&parser);
        return;
    }

    envRuleName = getenv("__GLVND_PROFILE_RULE");
    if (envRuleName != NULL) {
        success = ResolveRule(&parser, envRuleName);
    } else {
        int i;
        for (i=0; parser.configDirs[i] != NULL; i++) {
            GLboolean success = ParseProfileDir(&parser, parser.configDirs[i]);
            if (!success || parser.overrideProfile) {
                break;
            }
        }
    }

    if (success) {
        glvnd_list_for_each_entry(vendor, &parser.vendors, entry) {
            if (!vendor->disabled) {
                GLVNDappProfileVendor *profileVendor =
                    glvndProfileAddVendor(profile, vendor->name, vendor->data);
                if (profileVendor != NULL) {
                    profileVendor->onlyInServerList = vendor->onlyInServerList;
                }
            }
        }
    }
    FreeParserState(&parser);
}
