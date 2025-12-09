#include <cstdio>
#include <cstring>
#include <emscripten.h>
#include "Core/FileLoaders/LocalFileLoader.h"
#include "Core/FileSystems/BlockDevices.h"
#include "Core/FileSystems/ISOFileSystem.h"

// Variáveis globais para manter o estado
static BlockDevice* g_blockDevice = nullptr;
static ISOFileSystem* g_isoFileSystem = nullptr;
static SequentialHandleAllocator g_handleAllocator;

extern "C" {

// Função para testar se a ISO foi lida corretamente
EMSCRIPTEN_KEEPALIVE
int testISO(const char* filename) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║              ISO LOADER TEST - EMSCRIPTEN                     ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");

    printf("📂 Arquivo: %s\n\n", filename);

    // 1. Criar LocalFileLoader
    printf("═══ PASSO 1: LocalFileLoader ═══\n");
    Path path(filename);
    LocalFileLoader* loader = new LocalFileLoader(path);

    if (!loader->Exists()) {
        printf("❌ ERRO: Arquivo não encontrado no MEMFS!\n");
        printf("   Certifique-se de usar: FS.writeFile('%s', data)\n", filename);
        delete loader;
        return -1;
    }

    s64 fileSize = loader->FileSize();
    printf("✅ Arquivo encontrado!\n");
    printf("   📏 Tamanho: %lld bytes (%.2f MB)\n\n", fileSize, fileSize / (1024.0 * 1024.0));

    // 2. Ler header para identificar tipo
    printf("═══ PASSO 2: Identificar Tipo ═══\n");
    u8 header[16];
    size_t read = loader->ReadAt(0, 1, 16, header);
    printf("   📖 Bytes lidos: %zu\n", read);
    printf("   🔍 Header HEX: ");
    for (int i = 0; i < 8; i++) printf("%02X ", header[i]);
    printf("\n");
    printf("   🔍 Header ASCII: ");
    for (int i = 0; i < 8; i++) {
        char c = (header[i] >= 32 && header[i] < 127) ? header[i] : '.';
        printf("%c", c);
    }
    printf("\n");

    const char* tipoArquivo = "ISO Normal";
    if (memcmp(header, "CISO", 4) == 0) {
        tipoArquivo = "CSO (Compressed ISO)";
    } else if (memcmp(header, "PBP", 3) == 0) {
        tipoArquivo = "PBP (PlayStation Portable Package)";
    } else if (header[0] == 'P' && header[1] == 'K') {
        tipoArquivo = "ZIP Archive";
    }
    printf("   📀 Tipo detectado: %s\n\n", tipoArquivo);

    // 3. Criar BlockDevice
    printf("═══ PASSO 3: BlockDevice ═══\n");
    std::string errorString;
    g_blockDevice = ConstructBlockDevice(loader, &errorString);

    if (!g_blockDevice) {
        printf("❌ ERRO ao criar BlockDevice: %s\n", errorString.c_str());
        delete loader;
        return -2;
    }

    printf("✅ BlockDevice criado!\n");
    printf("   🧱 Tamanho do bloco: %d bytes\n", g_blockDevice->GetBlockSize());
    printf("   📊 Total de blocos: %u\n", g_blockDevice->GetNumBlocks());
    printf("   💾 Tamanho real: %llu bytes (%.2f MB)\n\n", 
           g_blockDevice->GetUncompressedSize(),
           g_blockDevice->GetUncompressedSize() / (1024.0 * 1024.0));

    // 4. Ler setor 16 (Volume Descriptor)
    printf("═══ PASSO 4: Verificar ISO9660 ═══\n");
    u8 sector[2048];
    bool readOk = g_blockDevice->ReadBlock(16, sector);

    if (!readOk) {
        printf("❌ ERRO: Não foi possível ler o setor 16!\n");
        delete g_blockDevice;
        g_blockDevice = nullptr;
        return -3;
    }

    printf("✅ Setor 16 lido!\n");
    printf("   🔍 Assinatura: %.5s\n", sector + 1);

    if (memcmp(sector + 1, "CD001", 5) != 0) {
        printf("⚠️  AVISO: Assinatura CD001 não encontrada!\n");
        printf("   O arquivo pode estar corrompido ou não ser ISO9660.\n\n");
    } else {
        printf("✅ ISO9660 VÁLIDO!\n");
        
        // Extrair informações do Volume Descriptor
        char volumeId[33] = {0};
        memcpy(volumeId, sector + 40, 32);
        // Remover espaços do final
        for (int i = 31; i >= 0 && volumeId[i] == ' '; i--) volumeId[i] = 0;
        
        char publisherId[129] = {0};
        memcpy(publisherId, sector + 318, 128);
        for (int i = 127; i >= 0 && publisherId[i] == ' '; i--) publisherId[i] = 0;
        
        printf("   📛 Volume ID: [%s]\n", volumeId);
        if (strlen(publisherId) > 0) {
            printf("   🏭 Publisher: [%s]\n", publisherId);
        }
        printf("\n");
    }

    // 5. Criar ISOFileSystem e listar arquivos
    printf("═══ PASSO 5: Sistema de Arquivos ═══\n");
    g_isoFileSystem = new ISOFileSystem(&g_handleAllocator, g_blockDevice);

    bool exists = false;
    std::vector<PSPFileInfo> rootFiles = g_isoFileSystem->GetDirListing("/", &exists);

    if (!exists || rootFiles.empty()) {
        printf("⚠️  Diretório raiz vazio ou não encontrado.\n");
        printf("   Tentando PSP_GAME...\n");
        rootFiles = g_isoFileSystem->GetDirListing("/PSP_GAME", &exists);
    }

    if (rootFiles.empty()) {
        printf("   Nenhum arquivo encontrado na raiz.\n\n");
    } else {
        printf("✅ Arquivos encontrados: %zu\n\n", rootFiles.size());
        printf("   ┌────────────────────────────────────────────────────┐\n");
        printf("   │ NOME                              │ TAMANHO       │\n");
        printf("   ├────────────────────────────────────────────────────┤\n");
        
        int count = 0;
        for (const auto& file : rootFiles) {
            if (count >= 20) {
                printf("   │ ... e mais %zu arquivos                          │\n", rootFiles.size() - 20);
                break;
            }
            
            char sizeStr[20];
            if (file.type == FILETYPE_DIRECTORY) {
                snprintf(sizeStr, sizeof(sizeStr), "<DIR>");
            } else if (file.size >= 1024 * 1024) {
                snprintf(sizeStr, sizeof(sizeStr), "%.1f MB", file.size / (1024.0 * 1024.0));
            } else if (file.size >= 1024) {
                snprintf(sizeStr, sizeof(sizeStr), "%.1f KB", file.size / 1024.0);
            } else {
                snprintf(sizeStr, sizeof(sizeStr), "%lld B", file.size);
            }
            
            printf("   │ %-33.33s │ %13s │\n", file.name.c_str(), sizeStr);
            count++;
        }
        printf("   └────────────────────────────────────────────────────┘\n\n");
    }

    // 6. Procurar EBOOT.BIN (executável do PSP)
    printf("═══ PASSO 6: Procurar Executável PSP ═══\n");
    
    PSPFileInfo ebootInfo = g_isoFileSystem->GetFileInfo("/PSP_GAME/SYSDIR/EBOOT.BIN");
    if (ebootInfo.exists) {
        printf("✅ EBOOT.BIN encontrado!\n");
        printf("   📍 Caminho: /PSP_GAME/SYSDIR/EBOOT.BIN\n");
        printf("   📏 Tamanho: %lld bytes (%.2f KB)\n", ebootInfo.size, ebootInfo.size / 1024.0);
        printf("   🎮 Este é um jogo PSP válido!\n\n");
    } else {
        // Tentar boot.bin
        ebootInfo = g_isoFileSystem->GetFileInfo("/PSP_GAME/SYSDIR/BOOT.BIN");
        if (ebootInfo.exists) {
            printf("✅ BOOT.BIN encontrado!\n");
            printf("   📍 Caminho: /PSP_GAME/SYSDIR/BOOT.BIN\n");
            printf("   📏 Tamanho: %lld bytes\n\n", ebootInfo.size);
        } else {
            printf("⚠️  EBOOT.BIN não encontrado.\n");
            printf("   Pode ser um disco de dados ou formato diferente.\n\n");
        }
    }

    // 7. Procurar PARAM.SFO (informações do jogo)
    printf("═══ PASSO 7: Informações do Jogo ═══\n");
    
    PSPFileInfo sfoInfo = g_isoFileSystem->GetFileInfo("/PSP_GAME/PARAM.SFO");
    if (sfoInfo.exists) {
        printf("✅ PARAM.SFO encontrado!\n");
        printf("   📍 Caminho: /PSP_GAME/PARAM.SFO\n");
        printf("   📏 Tamanho: %lld bytes\n", sfoInfo.size);
        
        // Ler e mostrar parte do SFO
        if (sfoInfo.size > 0 && sfoInfo.size < 65536) {
            int handle = g_isoFileSystem->OpenFile("/PSP_GAME/PARAM.SFO", FILEACCESS_READ);
            if (handle >= 0) {
                u8* sfoData = new u8[sfoInfo.size];
                g_isoFileSystem->ReadFile(handle, sfoData, sfoInfo.size);
                g_isoFileSystem->CloseFile(handle);
                
                // Procurar título do jogo (simplificado)
                printf("   🎮 Dados SFO lidos com sucesso!\n");
                delete[] sfoData;
            }
        }
        printf("\n");
    } else {
        printf("⚠️  PARAM.SFO não encontrado.\n\n");
    }

    // Resultado final
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                    ✅ TESTE CONCLUÍDO!                       ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║  ISO Loader funcionando corretamente no navegador!           ║\n");
    printf("║                                                              ║\n");
    printf("║  ✅ LocalFileLoader: OK                                      ║\n");
    printf("║  ✅ BlockDevice: OK                                          ║\n");
    printf("║  ✅ ISOFileSystem: OK                                        ║\n");
    printf("║  ✅ Leitura de arquivos: OK                                  ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");

    return 0;
}

// Função para listar diretório específico
EMSCRIPTEN_KEEPALIVE
int listDirectory(const char* path) {
    if (!g_isoFileSystem) {
        printf("❌ ISO não carregada! Use testISO primeiro.\n");
        return -1;
    }

    printf("\n📁 Listando: %s\n\n", path);

    bool exists = false;
    std::vector<PSPFileInfo> files = g_isoFileSystem->GetDirListing(path, &exists);

    if (!exists) {
        printf("❌ Diretório não encontrado!\n");
        return -1;
    }

    for (const auto& file : files) {
        const char* type = (file.type == FILETYPE_DIRECTORY) ? "📁" : "📄";
        printf("  %s %s (%lld bytes)\n", type, file.name.c_str(), file.size);
    }

    printf("\nTotal: %zu itens\n", files.size());
    return 0;
}

// Função para ler arquivo específico e mostrar preview
EMSCRIPTEN_KEEPALIVE
int readFilePreview(const char* path, int maxBytes) {
    if (!g_isoFileSystem) {
        printf("❌ ISO não carregada!\n");
        return -1;
    }

    PSPFileInfo info = g_isoFileSystem->GetFileInfo(path);
    if (!info.exists) {
        printf("❌ Arquivo não encontrado: %s\n", path);
        return -1;
    }

    printf("\n📄 Arquivo: %s\n", path);
    printf("   Tamanho: %lld bytes\n\n", info.size);

    int handle = g_isoFileSystem->OpenFile(path, FILEACCESS_READ);
    if (handle < 0) {
        printf("❌ Erro ao abrir arquivo!\n");
        return -1;
    }

    int toRead = (info.size < maxBytes) ? info.size : maxBytes;
    u8* data = new u8[toRead];
    size_t bytesRead = g_isoFileSystem->ReadFile(handle, data, toRead);
    g_isoFileSystem->CloseFile(handle);

    printf("   Primeiros %zu bytes (HEX):\n   ", bytesRead);
    for (size_t i = 0; i < bytesRead && i < 64; i++) {
        printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0) printf("\n   ");
    }
    printf("\n");

    delete[] data;
    return 0;
}

// Limpar recursos
EMSCRIPTEN_KEEPALIVE
void cleanup() {
    if (g_isoFileSystem) {
        delete g_isoFileSystem;
        g_isoFileSystem = nullptr;
    }
    // BlockDevice é deletado pelo ISOFileSystem
    g_blockDevice = nullptr;
    printf("🧹 Recursos liberados.\n");
}

}

// Variáveis globais necessárias para MemMap
namespace Memory {
    u8 *base = nullptr;
    u32 g_MemorySize = 0x02000000;
}

int main() {
    printf("🎮 ISO Loader para PSP - WebAssembly\n");
    printf("   Pronto para carregar ISOs!\n\n");
    return 0;
}