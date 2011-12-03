
#include "raspell.h"

extern void Init_dictinfo();
extern void Init_aspell();

void Init_raspell() {
    cAspellError = rb_define_class("AspellError", rb_eStandardError);
    Init_dictinfo();
    Init_aspell();
}

static AspellDictInfo* get_info(VALUE info) {
    AspellDictInfo *result;
    Data_Get_Struct(info, AspellDictInfo, result);
    return result;
}

static VALUE dictinfo_s_new(int argc, VALUE *argv, VALUE klass) {
    rb_raise(rb_eException, "not instantiable");
}

static VALUE dictinfo_name(VALUE self) {
    return rb_str_new2(get_info(self)->name);
}

static VALUE dictinfo_code(VALUE self) {
    return rb_str_new2(get_info(self)->code);
}

static VALUE dictinfo_jargon(VALUE self) {
    return rb_str_new2(get_info(self)->jargon);
}

static VALUE dictinfo_size(VALUE self) {
    return INT2FIX(get_info(self)->size);
}

static VALUE dictinfo_size_str(VALUE self) {
    return rb_str_new2(get_info(self)->size_str);
}

void Init_dictinfo() {
    //CLASS DEFINITION=========================================================
    cDictInfo = rb_define_class("AspellDictInfo", rb_cObject);

    //CLASS METHODS============================================================
    rb_define_singleton_method(cDictInfo, "new", dictinfo_s_new, 0);

    //METHODS =================================================================
    rb_define_method(cDictInfo, "name", dictinfo_name, 0);
    rb_define_method(cDictInfo, "code", dictinfo_code, 0);
    rb_define_method(cDictInfo, "jargon", dictinfo_jargon, 0);
    rb_define_method(cDictInfo, "size", dictinfo_size, 0);
    rb_define_method(cDictInfo, "size_str", dictinfo_size_str, 0);
}

extern VALUE rb_cFile;

/**
 * This method is called from the garbage collector during finalization.
 * @param p pointer to spellchecker-object.
 */
static void aspell_free(void *p) {
    delete_aspell_speller(p);
}

/**
 * Check for an error - raise exception if so.
 * @param speller the spellchecker-object.
 * @return void
 * @exception Exception if there was an error.
 */
static void check_for_error(AspellSpeller * speller) {
    if (aspell_speller_error(speller) != 0) {
        rb_raise(cAspellError, aspell_speller_error_message(speller));
    }
}

/**
 * Set a specific option, that is known by aspell.
 * @param config the config object of a specific spellchecker.
 * @param key the option to set (eg: lang).
 * @param value the value of the option to set (eg: "us_US").
 * @exception Exception if key not known, or value undefined.
 */
static void set_option(AspellConfig *config, char *key, char *value) {
    //printf("set option: %s = %s\n", key, value);
    if (aspell_config_replace(config, key, value) == 0) {
        rb_raise(cAspellError, aspell_config_error_message(config));
    }
    //check config:
    if (aspell_config_error(config) != 0) {
        rb_raise(cAspellError, aspell_config_error_message(config));
    }
}

static void set_options(AspellConfig *config, VALUE hash) {
    VALUE options = rb_funcall(hash, rb_intern("keys"), 0);
    int count=RARRAY_LEN(options);
    int c = 0;
    //set all values
    while(c<count) {
        //fetch option
        VALUE option = RARRAY_PTR(options)[c];
        VALUE value = rb_funcall(hash, rb_intern("fetch"), 1, option);
        if (TYPE(option)!=T_STRING) rb_raise(cAspellError, "Given key must be a string.");
        if (TYPE(value )!=T_STRING) rb_raise(cAspellError, "Given value must be a string.");
        set_option(config, StringValuePtr(option), StringValuePtr(value));
        c++;
    }
}

/**
 * Extract c-struct speller from ruby object.
 * @param speller the speller as ruby object.
 * @return the speller as c-struct.
 */
static AspellSpeller* get_speller(VALUE speller) {
    AspellSpeller *result;
    Data_Get_Struct(speller, AspellSpeller, result);
    return result;
}

/**
 * Generate a document checker object from a given speller.
 * @param speller the speller that shall chech a document.
 * @return a fresh document checker.
 */
static AspellDocumentChecker* get_checker(AspellSpeller *speller) {
    AspellCanHaveError * ret;
    AspellDocumentChecker * checker;
    ret = new_aspell_document_checker(speller);
    if (aspell_error(ret) != 0)
        rb_raise(cAspellError, aspell_error_message(ret));
    checker = to_aspell_document_checker(ret);
    return checker;
}

/**
 * Utility function that wraps a list of words as ruby array of ruby strings.
 * @param list an aspell wordlist.
 * @return an ruby array, containing all words as ruby strings.
 */
static VALUE get_list(const AspellWordList *list) {
    VALUE result = rb_ary_new2(aspell_word_list_size(list));
    if (list != 0) {
        AspellStringEnumeration * els = aspell_word_list_elements(list);
        const char * word;
        while ( (word = aspell_string_enumeration_next(els)) != 0) {
            rb_ary_push(result, rb_str_new2(word));
        }
        delete_aspell_string_enumeration(els);
    }
    return result;
}

/**
 * Generate a regexp from the given word with word boundaries.
 * @param word the word to match.
 * @return regular expression, matching exactly the word as whole.
 */
static VALUE get_wordregexp(VALUE word) {
    char *cword = StringValuePtr(word);
    char *result = malloc((strlen(cword)+5)*sizeof(char));
    *result='\0';
    strcat(result, "\\b");
    strcat(result, cword);
    strcat(result, "\\b");
    word = rb_reg_new(result, strlen(result), 0);
    free(result);
    return word;
}


/**
 * Ctor for aspell objects:
 * Aspell.new(language, jargon, size, encoding)
 * Please note: All parameters are optional. If a parameter is omitted, a default value is assumed from
 *              the environment (eg lang from $LANG). To retain default values, you can use nil
 *              as value: to set only size: Aspell.new(nil, nil, "80")
 * @param language ISO639 language code plus optional ISO 3166 counry code as string (eg: "de" or "us_US")
 * @param jargon a special jargon of the selected language
 * @param size the size of the dictionary to chose (if there are options)
 * @param encoding the encoding to use
 * @exception Exception if the specified dictionary is not found.
 */
static VALUE aspell_s_new(int argc, VALUE *argv, VALUE klass) {
    VALUE vlang, vjargon, vsize, vencoding;
    const char *tmp;
    //aspell values
    AspellCanHaveError * ret;
    AspellSpeller * speller;
    AspellConfig * config;

    //create new config
    config = new_aspell_config();

    //extract values
    rb_scan_args(argc, argv, "04", &vlang, &vjargon, &vsize, &vencoding);

    //language:
    if (RTEST(vlang)) set_option(config, "lang", StringValuePtr(vlang));
    //jargon:
    if (RTEST(vjargon)) set_option(config, "jargon", StringValuePtr(vjargon));
    //size:
    if (RTEST(vsize)) set_option(config, "size", StringValuePtr(vsize));
    //encoding:
    if (RTEST(vencoding)) set_option(config, "encoding", StringValuePtr(vencoding));

    //create speller:
    ret = new_aspell_speller(config);
    delete_aspell_config(config);
    if (aspell_error(ret) != 0) {
        tmp = strdup(aspell_error_message(ret));
        delete_aspell_can_have_error(ret);
        rb_raise(cAspellError, tmp);
    }

    speller = to_aspell_speller(ret);

    //wrap pointer
    return Data_Wrap_Struct(klass, 0, aspell_free, speller);
}



/**
 * Ctor for aspell objects.
 * This is a custom constructor and takes a hash of config options: key, value pairs.
 * Common use:
 *
 * a = Aspell.new({"lang"=>"de", "jargon"=>"berlin"})
 *
 * For a list of config options, see aspell manual.
 * @param options hash of options
 */
static VALUE aspell_s_new1(VALUE klass, VALUE options) {
    //aspell values
    AspellCanHaveError * ret;
    AspellSpeller * speller;
    AspellConfig * config;

    //create new config
    config = new_aspell_config();

    //set options
    set_options(config, options);

    //create speller:
    ret = new_aspell_speller(config);
    delete_aspell_config(config);
    if (aspell_error(ret) != 0) {
        const char *tmp = strdup(aspell_error_message(ret));
        delete_aspell_can_have_error(ret);
        rb_raise(cAspellError, tmp);
    }

    speller = to_aspell_speller(ret);

    //wrap pointer
    return Data_Wrap_Struct(klass, 0, aspell_free, speller);
}

/**
 * List all available dictionaries.
 * @param class object
 * @return array of AspellDictInfo objects.
 */
static VALUE aspell_s_list_dicts(VALUE klass) {
    AspellConfig * config;
    AspellDictInfoList * dlist;
    AspellDictInfoEnumeration * dels;
    const AspellDictInfo * entry;
    VALUE result = rb_ary_new();

    //get a list of dictionaries
    config = new_aspell_config();
    dlist = get_aspell_dict_info_list(config);
    delete_aspell_config(config);

    //iterate over list - fill ruby array
    dels = aspell_dict_info_list_elements(dlist);
    while ( (entry = aspell_dict_info_enumeration_next(dels)) != 0) {
        rb_ary_push(result, Data_Wrap_Struct(cDictInfo, 0, 0, (AspellDictInfo *)entry));
    }
    delete_aspell_dict_info_enumeration(dels);
    return result;
}

/**
 * @see set_option.
 */
static VALUE aspell_set_option(VALUE self, VALUE option, VALUE value) {
    AspellSpeller *speller = get_speller(self);
    set_option(aspell_speller_config(speller), StringValuePtr(option), StringValuePtr(value));
    return self;
}


/**
 * Delete an option.
 * @param option optionstring to remove from the options.
 */
static VALUE aspell_remove_option(VALUE self, VALUE option) {
    AspellSpeller *speller = get_speller(self);
    aspell_config_remove(aspell_speller_config(speller), StringValuePtr(option));
    return self;
}

/**
 * To set the mode, words are suggested.
 * @param one of Aspell::[ULTRA|FAST|NORMAL|BADSPELLERS]
 */
static VALUE aspell_set_suggestion_mode(VALUE self, VALUE value) {
    AspellSpeller *speller = get_speller(self);
    set_option(aspell_speller_config(speller), "sug-mode", StringValuePtr(value));
    return self;
}

/**
 * Returns the personal wordlist as array of strings.
 * @return array of strings
 */
static VALUE aspell_personal_wordlist(VALUE self) {
    AspellSpeller *speller = get_speller(self);
    return get_list(aspell_speller_personal_word_list(speller));
}

/**
 * Returns the session wordlist as array of strings.
 * @return array of strings
 */
static VALUE aspell_session_wordlist(VALUE self) {
    AspellSpeller *speller = get_speller(self);
    return get_list(aspell_speller_session_word_list(speller));
}

/**
 * Returns the main wordlist as array of strings.
 * @return array of strings
 */
static VALUE aspell_main_wordlist(VALUE self) {
    AspellSpeller *speller = get_speller(self);
    return get_list(aspell_speller_main_word_list(speller));
}

/**
 * Synchronize all wordlists with the current session.
 */
static VALUE aspell_save_all_wordlists(VALUE self) {
    AspellSpeller *speller = get_speller(self);
    aspell_speller_save_all_word_lists(speller);
    check_for_error(speller);
    return self;
}

/**
 * Remove all words inside session.
 */
static VALUE aspell_clear_session(VALUE self) {
    AspellSpeller *speller = get_speller(self);
    aspell_speller_clear_session(speller);
    check_for_error(speller);
    return self;
}

/**
 * Suggest words for the given misspelled word.
 * @param word the misspelled word.
 * @return array of strings.
 */
static VALUE aspell_suggest(VALUE self, VALUE word) {
    AspellSpeller *speller = get_speller(self);
    return get_list(aspell_speller_suggest(speller, StringValuePtr(word), -1));
}

/**
 * Add a given word to the list of known words inside my private dictionary.
 * You have to call aspell_save_all_wordlists to make sure the list gets persistent.
 * @param word the word to add.
 */
static VALUE aspell_add_to_personal(VALUE self, VALUE word) {
    AspellSpeller *speller = get_speller(self);
    aspell_speller_add_to_personal(speller, StringValuePtr(word), -1);
    check_for_error(speller);
    return self;
}

/**
 * Add a given word to the list of known words just for the lifetime of this object.
 * @param word the word to add.
 */
static VALUE aspell_add_to_session(VALUE self, VALUE word) {
    AspellSpeller *speller = get_speller(self);
    aspell_speller_add_to_session(speller, StringValuePtr(word), -1);
    check_for_error(speller);
    return self;
}

/**
 * Retrieve the value of a specific option.
 * The options are listed inside
 * Aspell::[DictionaryOptions|CheckerOptions|FilterOptions|RunTogetherOptions|MiscOptions|UtilityOptions]
 * @param word the option as string.
 */
static VALUE aspell_conf_retrieve(VALUE self, VALUE key) {
    AspellSpeller *speller = get_speller(self);
    AspellConfig *config = aspell_speller_config(speller);
    VALUE result = rb_str_new2(aspell_config_retrieve(config, StringValuePtr(key)));
    if (aspell_config_error(config) != 0) {
        rb_raise(cAspellError, aspell_config_error_message(config));
    }
    return result;
}

/**
 * Retrieve the value of a specific option as list.
 * @param word the option as string.
 */
static VALUE aspell_conf_retrieve_list(VALUE self, VALUE key) {
    AspellSpeller *speller = get_speller(self);
    AspellConfig *config = aspell_speller_config(speller);
    AspellStringList * list = new_aspell_string_list();
    AspellMutableContainer * container  = aspell_string_list_to_mutable_container(list);
    AspellStringEnumeration * els;
    VALUE result = rb_ary_new();
    const char *option_value;

    //retrieve list
    aspell_config_retrieve_list(config, StringValuePtr(key), container);
    //check for error
    if (aspell_config_error(config) != 0) {
        char *tmp = strdup(aspell_config_error_message(config));
        delete_aspell_string_list(list);
        rb_raise( cAspellError, tmp);
    }

    //iterate over list
    els = aspell_string_list_elements(list);
    while ( (option_value = aspell_string_enumeration_next(els)) != 0) {
        //push the option value to result
        rb_ary_push(result, rb_str_new2(option_value));
    }
    //free list
    delete_aspell_string_enumeration(els);
    delete_aspell_string_list(list);

    return result;
}

/**
 * Simply dump config.
 * Not very useful at all.
 */
static VALUE aspell_dump_config(VALUE self) {
    AspellSpeller *speller = get_speller(self);
    AspellConfig *config = aspell_speller_config(speller);
    AspellKeyInfoEnumeration * key_list = aspell_config_possible_elements( config, 0 );
    const AspellKeyInfo * entry;

    while ( (entry = aspell_key_info_enumeration_next(key_list) ) ) {
        printf("%20s:  %s\n", entry->name, aspell_config_retrieve(config, entry->name) );
    }
    delete_aspell_key_info_enumeration(key_list);
    return self;
}

/**
 * Check a given word for correctness.
 * @param word the word to check
 * @return true if the word is correct, otherwise false.
 */
static VALUE aspell_check(VALUE self, VALUE word) {
    AspellSpeller *speller = get_speller(self);
    VALUE result = Qfalse;
    int code = aspell_speller_check(speller, StringValuePtr(word), -1);
    if (code == 1)
        result = Qtrue;
    else if (code == 0)
        result = Qfalse;
    else
        rb_raise( cAspellError, aspell_speller_error_message(speller));
    return result;
}

/**
 * This method will check an array of strings for misspelled words.
 * This method needs a block to work proper. Each misspelled word is yielded,
 * a correct word as result from the block is assumed.
 * Common use:
 *
 * a = Aspell.new(...)
 * text = ...
 * a.correct_lines(text) { |badword|
 *    puts "Error: #{badword}\n"
 *    puts a.suggest(badword).join(" | ")
 *    gets #the input is returned as right word
 * }
 *
 * @param ary the array of strings to check.
 * @result an array holding all lines with corrected words.
 */
static VALUE aspell_correct_lines(VALUE self, VALUE ary) {
    VALUE result = ary;
    if (rb_block_given_p()) {
        //create checker
        AspellSpeller *speller = get_speller(self);
        AspellDocumentChecker * checker = get_checker(speller);
        AspellToken token;
        //some tmp values
        VALUE vline, sline;
        VALUE word, rword;
        char *line;
        int count=RARRAY_LEN(ary);
        int c=0;
        //create new result array
        result = rb_ary_new();
        //iterate over array
        while(c<count) {
            int offset=0;
            //fetch line
            vline = RARRAY_PTR(ary)[c];
            //save line
            sline = rb_funcall(vline, rb_intern("dup"), 0);
            //c representation
            line = StringValuePtr(vline);
            //process line
            aspell_document_checker_process(checker, line, -1);
            //iterate over all misspelled words
            while (token = aspell_document_checker_next_misspelling(checker), token.len != 0) {
                //extract word by start/length qualifier
                word = rb_funcall(vline, rb_intern("[]"), 2, INT2FIX(token.offset), INT2FIX(token.len));
                //get the right word from the block
                rword = rb_yield(word);
                //nil -> do nothing
                if(rword == Qnil) continue;
                //check for string
                if (TYPE(rword) != T_STRING) rb_raise(cAspellError, "Need a String to substitute");
                //chomp the string
                rb_funcall(rword, rb_intern("chomp!"), 0);
                //empty string -> do nothing
                if(strlen(StringValuePtr(rword)) == 0) continue;
                //remember word for later suggestion
                aspell_speller_store_replacement(speller, StringValuePtr(word), -1, StringValuePtr(rword), -1);
                //substitute the word by replacement
                rb_funcall(sline, rb_intern("[]="), 3, INT2FIX(token.offset+offset), INT2FIX(token.len), rword);
                //adjust offset
                offset += strlen(StringValuePtr(rword))-strlen(StringValuePtr(word));
                //printf("replace >%s< with >%s< (offset now %d)\n", StringValuePtr(word), StringValuePtr(rword), offset);
            }
            //push the substituted line to result
            rb_ary_push(result, sline);
            c++;
        }
        //free checker
        delete_aspell_document_checker(checker);
    } else {
        rb_raise(cAspellError, "No block given. How to correct?");
    }
    return result;
}

/**
 * Remember a correction.
 * This affects the suggestion of other words to fit this correction.
 * @param badword the bad spelled word as string.
 * @param badword the correction of the bad spelled word as string.
 * @result self
 */
static VALUE aspell_store_replacement(VALUE self, VALUE badword, VALUE rightword) {
    AspellSpeller *speller = get_speller(self);
    aspell_speller_store_replacement(speller, StringValuePtr(badword), -1, StringValuePtr(rightword), -1);
    return self;
}

/**
 * Simple utility function to correct a file.
 * The file gets read, content will be checked and write back.
 * Please note: This method will change the file! - no backup and of course: no warranty!
 * @param filename the name of the file as String.
 * @exception Exception due to lack of read/write permissions.
 */
static VALUE aspell_correct_file(VALUE self, VALUE filename) {
    if (rb_block_given_p()) {
        VALUE content = rb_funcall(rb_cFile, rb_intern("readlines"), 1, filename);
        VALUE newcontent = aspell_correct_lines(self, content);
        VALUE file = rb_file_open(StringValuePtr(filename), "w+");
        rb_funcall(file, rb_intern("write"), 1, newcontent);
        rb_funcall(file, rb_intern("close"), 0);
    } else {
        rb_raise(cAspellError, "No block given. How to correct?");
    }
    return self;

}

/**
 * Return a list of all misspelled words inside a given array of strings.
 * @param ary an array of strings to check for.
 * @return array of strings: words that are misspelled.
 */
static VALUE aspell_list_misspelled(VALUE self, VALUE ary) {
    Check_Type(ary, T_ARRAY);
    VALUE result = rb_hash_new();
    //create checker
    AspellSpeller *speller = get_speller(self);
    AspellDocumentChecker * checker = get_checker(speller);
    AspellToken token;
    VALUE word, vline;
    int count=RARRAY_LEN(ary);
    int c=0;
    //iterate over array
    while(c<count) {
        //process line
        vline = RARRAY_PTR(ary)[c];
        Check_Type(vline, T_STRING);
        aspell_document_checker_process(checker, StringValuePtr(vline), -1);
        //iterate over all misspelled words
        while (token = aspell_document_checker_next_misspelling(checker), token.len != 0) {
            //extract word by start/length qualifier
            word = rb_funcall(vline, rb_intern("[]"), 2, INT2FIX(token.offset), INT2FIX(token.len));
            rb_hash_aset(result, word, Qnil);
            //yield block, if given
            if (rb_block_given_p())
                rb_yield(word);
        }
        c++;
    }
    //free checker
    delete_aspell_document_checker(checker);
    result = rb_funcall(result, rb_intern("keys"), 0);
    return result;
}

void Init_aspell() {
    //CLASS DEFINITION=========================================================
    cAspell = rb_define_class("Aspell", rb_cObject);

    //CONSTANTS================================================================
    rb_define_const(cAspell, "ULTRA", rb_str_new2("ultra"));
    rb_define_const(cAspell, "FAST", rb_str_new2("fast"));
    rb_define_const(cAspell, "NORMAL", rb_str_new2("normal"));
    rb_define_const(cAspell, "BADSPELLERS", rb_str_new2("bad-spellers"));
    rb_define_const(cAspell, "DictionaryOptions", rb_ary_new3(  11,
                    rb_str_new2("master"),
                    rb_str_new2("dict-dir"),
                    rb_str_new2("lang"),
                    rb_str_new2("size"),
                    rb_str_new2("jargon"),
                    rb_str_new2("word-list-path"),
                    rb_str_new2("module-search-order"),
                    rb_str_new2("personal"),
                    rb_str_new2("repl"),
                    rb_str_new2("extra-dicts"),
                    rb_str_new2("strip-accents")));
    rb_define_const(cAspell, "CheckerOptions", rb_ary_new3(  11,
                    rb_str_new2("ignore"),
                    rb_str_new2("ignore-case"),
                    rb_str_new2("ignore-accents"),
                    rb_str_new2("ignore-repl"),
                    rb_str_new2("save-repl"),
                    rb_str_new2("sug-mode"),
                    rb_str_new2("module-search-order"),
                    rb_str_new2("personal"),
                    rb_str_new2("repl"),
                    rb_str_new2("extra-dicts"),
                    rb_str_new2("strip-accents")));
    rb_define_const(cAspell, "FilterOptions", rb_ary_new3(  10,
                    rb_str_new2("filter"),
                    rb_str_new2("mode"),
                    rb_str_new2("encoding"),
                    rb_str_new2("add-email-quote"),
                    rb_str_new2("rem-email-quote"),
                    rb_str_new2("email-margin"),
                    rb_str_new2("sgml-check"),
                    rb_str_new2("sgml-extension"),
                    rb_str_new2("tex-command"),
                    rb_str_new2("tex-check-command")));
    rb_define_const(cAspell, "RunTogetherOptions", rb_ary_new3( 3,
                    rb_str_new2("run-together"),
                    rb_str_new2("run-together-limit"),
                    rb_str_new2("run-together-min")));
    rb_define_const(cAspell, "MiscOptions", rb_ary_new3( 8,
                    rb_str_new2("conf"),
                    rb_str_new2("conf-dir"),
                    rb_str_new2("data-dir"),
                    rb_str_new2("local-data-dir"),
                    rb_str_new2("home-dir"),
                    rb_str_new2("per-conf"),
                    rb_str_new2("prefix"),
                    rb_str_new2("set-prefix")));

    rb_define_const(cAspell, "UtilityOptions", rb_ary_new3( 4,
                    rb_str_new2("backup"),
                    rb_str_new2("time"),
                    rb_str_new2("reverse"),
                    rb_str_new2("keymapping")));

    //CLASS METHODS============================================================
    rb_define_singleton_method(cAspell, "new", aspell_s_new, -1);
    rb_define_singleton_method(cAspell, "new1", aspell_s_new1, 1);
    rb_define_singleton_method(cAspell, "list_dicts", aspell_s_list_dicts, 0);

    //METHODS =================================================================
    rb_define_method(cAspell, "add_to_personal", aspell_add_to_personal, 1);
    rb_define_method(cAspell, "add_to_session", aspell_add_to_personal, 1);
    rb_define_method(cAspell, "check", aspell_check, 1);
    rb_define_method(cAspell, "correct_lines", aspell_correct_lines, 1);
    rb_define_method(cAspell, "correct_file", aspell_correct_file, 1);
    rb_define_method(cAspell, "clear_session", aspell_clear_session, 0);
    rb_define_method(cAspell, "dump_config", aspell_dump_config, 0);
    rb_define_method(cAspell, "list_misspelled", aspell_list_misspelled, 1);
    //This seems not to be very useful ...
    //rb_define_method(cAspell, "main_wordlist", aspell_main_wordlist, 0);
    rb_define_method(cAspell, "personal_wordlist", aspell_personal_wordlist, 0);
    rb_define_method(cAspell, "save_all_word_lists", aspell_save_all_wordlists, 0);
    rb_define_method(cAspell, "session_wordlist", aspell_session_wordlist, 0);
    rb_define_method(cAspell, "set_option", aspell_set_option, 2);
    rb_define_method(cAspell, "store_replacement", aspell_store_replacement, 2);
    rb_define_method(cAspell, "remove_option", aspell_remove_option, 1);
    rb_define_method(cAspell, "get_option", aspell_conf_retrieve, 1);
    rb_define_method(cAspell, "get_option_as_list", aspell_conf_retrieve_list, 1);
    rb_define_method(cAspell, "suggest", aspell_suggest, 1);
    rb_define_method(cAspell, "suggestion_mode=", aspell_set_suggestion_mode, 1);
}


