#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <gmp.h>
#include <stdbool.h>
#include <stdlib.h>
#include <gcrypt.h>


const unsigned int charInLong = sizeof(unsigned long) / sizeof(char);  // 4
const unsigned int blockSizeIn = 22 / sizeof(unsigned long);  // 5
const unsigned int charBlockSizeIn = charInLong * blockSizeIn;  // 20

static inline bool isCoprime(mpz_t x, mpz_t y)
{
    mpz_t a, b, c;
    mpz_init_set(a, x);
    mpz_init_set(b, y);
    mpz_init(c);
  
    while(mpz_cmp_si(b, 0))
    {
        mpz_mod(c, a, b);
        mpz_set(a, b);
        mpz_set(b, c);
    }
    bool result = !mpz_cmp_si(a, 1);
  
    mpz_clear(a);
    mpz_clear(b);
    mpz_clear(c);
  
    return result;
}

static inline int numberOfChars(mpz_t x)
{
    mpz_t a, b;
    mpz_init(a);
    mpz_init(b);
  
    int number = 1;
  
    while (1)
    {
        mpz_ui_pow_ui(a, 10, number-1);
        mpz_ui_pow_ui(b, 10, number);

        if (mpz_cmp(x, a)>=0 && mpz_cmp(x, b)<0)
            break;

        ++number;
    }
  
    mpz_clear(a);
    mpz_clear(b);
  
    return number;
}

char** getKeys()
{
    mpz_t N, d, e, s;
  
    mpz_t maxN, P, Q;
  
    mpz_init(maxN);
    
    char* rand = (char*)gcry_random_bytes(1, GCRY_STRONG_RANDOM);
    if (*rand < 0)
        *rand = -(*rand);
    
    mpz_ui_pow_ui(maxN, 10, (*rand)%20+15);
  
    gmp_randstate_t state;
    gmp_randinit_mt(state);
    gmp_randseed_ui(state, *rand);
  
    mpz_init(P);
    mpz_urandomm(P, state, maxN);
    mpz_clear(maxN);

    mpz_nextprime(P, P);
  

    int qDegree = 51 - numberOfChars(P);
    mpz_init(Q);
    mpz_ui_pow_ui(Q, 10, qDegree);
    mpz_nextprime(Q, Q);
  
    mpz_init(N);
    mpz_mul(N, P, Q);
    
    mpz_init(d);
    mpz_sub_ui(P, P, 1);
    mpz_sub_ui(Q, Q, 1);
    mpz_mul(d, P, Q);
    
    mpz_clear(Q);
    mpz_clear(P);
    
    mpz_init(s);
  
    do {
        mpz_urandomm(s, state, d);
    } while(!isCoprime(d, s));
    
    
    
    mpz_t r, newt, newr, temp, quotient;
    
    mpz_init(temp);
    mpz_init(quotient);
    mpz_init_set_si(e, 0); //t
    mpz_init_set(r, d); //n
    mpz_init_set_si(newt, 1);
    mpz_init_set(newr, s); //a
      
    while (mpz_cmp_si(newr , 0))
    {
        mpz_tdiv_q(quotient, r, newr);
        
        mpz_set(temp, newt);
        mpz_mul(newt, quotient, newt);
        mpz_sub(newt, e, newt);
        mpz_set(e, temp);
        
        mpz_set(temp, newr);
        mpz_mul(newr, quotient, newr);
        mpz_sub(newr, r, newr);
        mpz_set(r, temp);
    }
    
    if (mpz_cmp_si(r, 1) > 0)
    {
        printf("\nне обратимо\n");
        return NULL;
    }
    if (mpz_cmp_si(e, 0) < 0)
    {
        mpz_add(e, e, d);
    }
    
    mpz_clear(r);
    mpz_clear(newt);
    mpz_clear(newr);
    mpz_clear(temp);
    mpz_clear(quotient);
    
    char** result = (char**)malloc(3 * sizeof(char*));
    result[0] = mpz_get_str(NULL, 10, N);
    result[1] = mpz_get_str(NULL, 10, s);
    result[2] = mpz_get_str(NULL, 10, e);
    
    mpz_clear(N);
    mpz_clear(d);
    mpz_clear(s);
    mpz_clear(e);
    
    return result;
}

static inline bool isDigit(const char* s)
{
    int len = strlen(s);
    for (int i = 0; i < len; ++i)
    {
        if (s[i]>'9' || s[i]<'0')
            return false;
    }
    return true;
}


void closeApp(GtkWidget *window, gpointer data)
{
    gtk_main_quit();
}

void errorDialog(const char* errorText)
{
    GtkWidget* dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Error");
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), errorText, NULL);
    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}


void getKeysClick(GtkWidget *button, gpointer data)
{
    char** keys = getKeys();
    GtkEntry** entries = (GtkEntry**)data;
    gtk_entry_set_text(entries[0], keys[0]);
    gtk_entry_set_text(entries[1], keys[1]);
    gtk_entry_set_text(entries[2], keys[2]);
    free(keys);
}

void encryptClick(GtkWidget *button, gpointer data)
{
    const char *n_text = gtk_entry_get_text(GTK_ENTRY(((GtkEntry**)data)[0]));
    const char *s_text = gtk_entry_get_text(GTK_ENTRY(((GtkEntry**)data)[1]));
        
    if (!strlen(n_text) || !strlen(s_text) || !isDigit(n_text) || !isDigit(s_text))
    {
        errorDialog("Требуется ввести числовые значение N, s");
        return;
    }
    
    GtkTextBuffer* bufferEncrypt = ((GtkTextBuffer**)data)[2];
    GtkTextBuffer* bufferDecrypt = ((GtkTextBuffer**)data)[3];
    
    GtkTextIter start, end;
    
    gtk_text_buffer_get_bounds(bufferEncrypt, &start, &end);
    const char *textEncrypt = gtk_text_buffer_get_text(bufferEncrypt, &start, &end, TRUE);
        
    unsigned int lenIn = strlen(textEncrypt) + 1;
    
    unsigned long blockIn[blockSizeIn];
    for (unsigned char j = 0; j < blockSizeIn; ++j)
        blockIn[j] = 0L;
      
    
    mpz_t z, N, s;
    mpz_init(z);
    mpz_init_set_str(N, n_text, 10);
    mpz_init_set_str(s, s_text, 10);
    
    gtk_text_buffer_set_text(bufferDecrypt, "", -1);
    
    GtkTextIter iter;
    gtk_text_buffer_get_start_iter(bufferDecrypt, &iter);
    
    for(size_t i = 0; i<lenIn; ++i)
    {
        ((char*)blockIn)[i%charBlockSizeIn] = textEncrypt[i];
        if (i % charBlockSizeIn == charBlockSizeIn - 1 || i == lenIn - 1)
        {
            mpz_import(z, blockSizeIn, -1, sizeof(unsigned long), -1, 0, blockIn);
            mpz_powm(z, z, s, N);
            
            char* re = mpz_get_str(NULL, 36, z);
            gtk_text_buffer_insert(bufferDecrypt, &iter, re, -1);
            
            if (i != lenIn - 1)
                gtk_text_buffer_insert(bufferDecrypt, &iter, " ", -1);
            
            for (unsigned char j = 0; j < blockSizeIn; ++j)
                blockIn[j] = 0L;
        }
    }
    mpz_clear(z);
    mpz_clear(N);
    mpz_clear(s);
}

void decryptClick(GtkWidget *button, gpointer data)
{
    const char *n_text = gtk_entry_get_text(GTK_ENTRY(((GtkEntry**)data)[0]));
    const char *e_text = gtk_entry_get_text(GTK_ENTRY(((GtkEntry**)data)[1]));
    
    if (!strlen(n_text) || !strlen(e_text) || !isDigit(n_text) || !isDigit(e_text))
    {
        errorDialog("Требуется ввести числовые значения N, e");
        return;
    }
    
    GtkTextBuffer* bufferEncrypt = ((GtkTextBuffer**)data)[2];
    GtkTextBuffer* bufferDecrypt = ((GtkTextBuffer**)data)[3];
    
    GtkTextIter start, end;
    
    gtk_text_buffer_get_bounds(bufferDecrypt, &start, &end);
    char *textDecrypt = gtk_text_buffer_get_text(bufferDecrypt, &start, &end, FALSE);
        
    unsigned int lenIn = strlen(textDecrypt) + 1;
    
    unsigned long blockIn[blockSizeIn];
    for (unsigned char j = 0; j < blockSizeIn; ++j)
        blockIn[j] = 0L;
      
    char blockOut[charBlockSizeIn];
    
    mpz_t z, N, e;
    mpz_init(z);
    mpz_init_set_str(N, n_text, 10);
    mpz_init_set_str(e, e_text, 10);
    
    gtk_text_buffer_set_text(bufferEncrypt, "", -1);
    
    GtkTextIter iter;
    gtk_text_buffer_get_start_iter(bufferEncrypt, &iter);
    
    int strStart = 0, strEnd = 0;
    char memChar[2] = {0, 0};
    
    for (int i = 0; i<lenIn; ++i)
    {
        if (textDecrypt[i] == ' ' || i==lenIn-1)
        {
            for (int j = 0; j<charBlockSizeIn; ++j)
                blockOut[j] = 0;
            
            strEnd = i;
            textDecrypt[i] = '\0';
            char* newStr = textDecrypt+strStart;
            
            mpz_set_str(z, newStr, 36);
            mpz_powm(z, z, e, N);
            
            mpz_export(blockOut, NULL, -1, charBlockSizeIn*sizeof(char), -1, 0, z);
            
            for (int j = 0; j<charBlockSizeIn;)
            {
                if (memChar[0])
                {
                    memChar[1] = blockOut[j];
                    gtk_text_buffer_insert(bufferEncrypt, &iter, memChar, 2);
                    ++j;
                    memChar[0] = memChar[1] = 0;
                    continue;
                }
                if (blockOut[j]>0)
                {
                    gtk_text_buffer_insert(bufferEncrypt, &iter, blockOut+j, 1);
                    ++j;
                    continue;
                }
                if (blockOut[j]==0)
                {
                    goto stop;
                }
                if (j<charBlockSizeIn-1)
                {
                    gtk_text_buffer_insert(bufferEncrypt, &iter, blockOut+j, 2);
                    j += 2;
                }
                else
                {
                    memChar[0] = blockOut[j];
                    ++j;
                }
            }
            strStart = i = strEnd+1;
        }
    }
    stop:;
    mpz_clear(z);
    mpz_clear(N);
    mpz_clear(e);
}


int main(int argc, char *argv[])
{
    setlocale(LC_ALL,"");
    
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "RSA");
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(window), 1280, 720);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data (provider, "window{background-color: #dddddd;}", -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        (GtkStyleProvider*)provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        
    GtkIconTheme *icon_theme;
    GdkPixbuf *pixbuf;
    icon_theme = gtk_icon_theme_get_default();
    pixbuf = gtk_icon_theme_load_icon(icon_theme, "icon.ico", 128, 0, NULL);
    if (pixbuf)
        gtk_window_set_icon((GtkWindow*)window, pixbuf);
    
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(closeApp), NULL);

    GtkWidget *n_entry = gtk_entry_new();
    GtkWidget *s_entry = gtk_entry_new();
    GtkWidget *e_entry = gtk_entry_new();
    GtkWidget *n_label = gtk_label_new("N:");
    GtkWidget *s_label = gtk_label_new("s:");
    GtkWidget *e_label = gtk_label_new("e:");
    gtk_entry_set_placeholder_text(GTK_ENTRY(n_entry), "N");
    gtk_entry_set_placeholder_text(GTK_ENTRY(s_entry), "s");
    gtk_entry_set_placeholder_text(GTK_ENTRY(e_entry), "e");
    
    GtkWidget *keysButton = gtk_button_new_with_label("Получить ключи");
    
    gpointer keysEntries[3] = {n_entry, s_entry, e_entry};
    
    g_signal_connect(G_OBJECT(keysButton), "clicked", G_CALLBACK(getKeysClick), keysEntries);
    
    GtkWidget *vboxGl = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *hboxKeys = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    
    gtk_box_pack_start(GTK_BOX(hboxKeys), n_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hboxKeys), n_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hboxKeys), s_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hboxKeys), s_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hboxKeys), e_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hboxKeys), e_entry, TRUE, TRUE, 0);
    
    GtkWidget *hboxViews = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *vboxEncrypt = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *vboxDecrypt = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(hboxViews), vboxEncrypt, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hboxViews), vboxDecrypt, TRUE, TRUE, 0);
    
    GtkWidget *viewEncrypt = gtk_text_view_new();
    GtkWidget *viewDecrypt = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(viewEncrypt), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(viewDecrypt), GTK_WRAP_WORD_CHAR);
    gtk_widget_set_size_request(GTK_WIDGET(viewEncrypt), 625, 100);
    gtk_widget_set_size_request(GTK_WIDGET(viewDecrypt), 625, 100);
    GtkTextBuffer *bufferEncrypt = gtk_text_view_get_buffer(GTK_TEXT_VIEW(viewEncrypt));
    GtkTextBuffer *bufferDecrypt = gtk_text_view_get_buffer(GTK_TEXT_VIEW(viewDecrypt));
    
    GtkWidget *labelEncrypt = gtk_label_new("Исходный текст:");
    GtkWidget *labelDecrypt = gtk_label_new("Шифротекст:");
    
    GtkWidget *encryptButton = gtk_button_new_with_label("Шифрофание");
    GtkWidget *decryptButton = gtk_button_new_with_label("Дешифрование");
    
    gpointer encryptData[4] = {n_entry, s_entry, bufferEncrypt, bufferDecrypt};
    g_signal_connect(G_OBJECT(encryptButton), "clicked",
        G_CALLBACK(encryptClick), encryptData);
        
    gpointer decryptData[4] = {n_entry, e_entry, bufferEncrypt, bufferDecrypt};
    g_signal_connect(G_OBJECT(decryptButton), "clicked",
        G_CALLBACK(decryptClick), decryptData);
    
    gtk_box_pack_start(GTK_BOX(vboxEncrypt), labelEncrypt, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vboxEncrypt), viewEncrypt, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(vboxEncrypt), encryptButton, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vboxDecrypt), labelDecrypt, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vboxDecrypt), viewDecrypt, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(vboxDecrypt), decryptButton, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(vboxGl), hboxKeys, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vboxGl), keysButton, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vboxGl), hboxViews, TRUE, TRUE, 0);
    
    gtk_container_add(GTK_CONTAINER(window), vboxGl);
    
    gtk_widget_show_all(window);
    
    gtk_main();
    
    return 0;
}
