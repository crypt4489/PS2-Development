#ifndef PS_GLOBAL_H
#define PS_GLOBAL_H

#include <stdbool.h>

#include <packet.h>
#include <draw_blending.h>
#include <draw_buffers.h>
#include <draw_primitives.h>
#include <draw_sampling.h>

#include "ps_renderflags.h"

#define DEFAULT_PIPELINE_SIZE 200

#define MAX_CHAR_TEXTURE_NAME 20
#define MAX_CHAR_PIPELINE_NAME 20
#define MAX_FILE_NAME 35
#define MAX_ANIMATION_NAME 48
#define MAX_JOINT_NAME 48

#define DRAW_DISABLE 0
#define DRAW_ENABLE 1

typedef float VECTOR[4] __attribute__((__aligned__(16)));

typedef float MATRIX[16] __attribute__((__aligned__(16)));

typedef s32 VectorInt[4] __attribute__((__aligned__(16)));

typedef union {
	u64 rgbaq;
	struct {
		u8 r;
		u8 g;
		u8 b;
		u8 a;
		float q;
	};
} __attribute__((packed,aligned(8))) Color;

typedef struct vu1_program_t
{
    union
    {
        struct
        {
            unsigned int size : 12;
            unsigned int address : 12;
            unsigned int stage : 2;
            unsigned int pad : 6;
        };
        u32 programInfo;
    };
    u32 programId;
    u32 *codeStart;
    u32 *codeEnd;

} VU1Program;

#define MAX_VU1_CODE_ADDRESS 2048

typedef struct vu1_manager_t
{
    u16 programsInVU1;
    u16 programIdentifier;
    u32 basePointer;
    u32 numPrograms;
    VU1Program **programs;
} VU1Manager;

enum VU1Programs
{
    VU1GenericStage4 = 0,
    VU1GenericLight3D = 1,
    VU1GenericMorph = 2,
    VU1GenericEnvMap,
    VU1GenericAnimTex,
    VU1GenericSpecular,
    VU1GenericClipper,
    VU1GenericSkinned
};




#define GEN_PIPELINE_NAME "GEN_PIPELINE"
#define LIGHT_PIPELINE_NAME "LIGHT_PIPELINE"

#define SWAP_ENDIAN(num)          \
    ((num >> 24) & 0xff) |        \
        ((num << 8) & 0xff0000) | \
        ((num >> 8) & 0xff00) |   \
        ((num << 24) & 0xff000000)

enum DMATAG_CODES
{
    DMA_CNT = 1,
    DMA_CNTS = 0, // destination tag only
    DMA_NEXT = 2,
    DMA_END = 7,
    DMA_CALL = 5,
    DMA_RET = 6,
    DMA_REF = 3,
    DMA_REFS = 4,
    DMA_REFE = 0
};

typedef struct linked_list_t
{
    struct linked_list_t *next;
    void *data;
} LinkedList;

enum QueueType
{
    FIFO = 1,
    LIFO = 2,
};

typedef struct queue_t
{
    u32 count;
    u32 maxCount;
    u32 type;
    LinkedList *top;
} Queue;

typedef union Bin2Float_t
{
    u32 int_x;
    float float_x;
} Bin2Float;

enum TextureReadType
{
    READ_BMP = 0,
    READ_PNG = 1
};

#define TEX_ADDRESS_CLAMP 0
#define TEX_ADDRESS_WRAP 1

#define PS_TEX_MEMORY 0
#define PS_TEX_VRAM 1

typedef struct
{
    char name[MAX_CHAR_TEXTURE_NAME];
    texbuffer_t texbuf;
    clutbuffer_t clut;
    lod_t lod;
    unsigned char *pixels;
    unsigned char *clut_buffer;
    u32 width;
    u32 height;
    u32 psm;
    u64 id;
    u16 mode;
    u16 type;
    u16 mipLevels;
    u16 pad1;
    LinkedList *mipMaps;
} Texture;


typedef struct
{
    VECTOR p1, p2;
} Line;

typedef struct 
{
    VECTOR origin;
    VECTOR direction;
} Ray;

enum ObjectBoundingTypes
{
    VBO_FIT = 0, //aabb in world space
    VBO_FIXED = 1, //rotated aabb in world space
    VBO_SPHERE = 2,
    VBO_OBB = 3
};

typedef struct
{
    u32 type;
    void *vbo;
} ObjectBounds;

typedef struct
{
    VECTOR top;
    VECTOR bottom;
} BoundingBox;

typedef struct
{
    VECTOR center;
    float radius;
} BoundingSphere;

typedef struct
{
    VECTOR center;
    VECTOR axes[3];
    VECTOR halfwidths;
} BoundingOrientBox;

struct morph_target_handle_t;
typedef struct morph_target_handle_t MorphTargetBuffer;
struct interpolater_node_t;
typedef struct interpolater_node_t Interpolator;

struct material_node_t;
typedef struct material_node_t Material;
struct mesh_buffers_t;
typedef struct mesh_buffers_t MeshBuffers;
struct joint_t;
typedef struct joint_t Joint;
struct animation_node_t;
typedef struct animation_node_t AnimationNode;
struct animation_key_t;
typedef struct animation_key_t AnimationKey;
struct animation_key_holder_t;
typedef struct animation_key_holder_t AnimationKeyHolder;
struct animation_data_t;
typedef struct animation_data_t AnimationData;
struct animation_mesh_t;
typedef struct animation_mesh_t AnimationMesh;
struct animator_t;
typedef struct animator_t Animator;
struct pipelinecblist_t;
typedef struct pipelinecblist_t PipelineCallback;
struct vu_pipeline_renderpass_t;
typedef struct vu_pipeline_renderpass_t VU1PipelineRenderPass;
struct vu_pipeline_t;
typedef struct vu_pipeline_t VU1Pipeline;
struct gameobject_t;
typedef struct gameobject_t GameObject;
struct render_world_t;
typedef struct render_world_t RenderWorld;
struct worldcblist_t;
typedef struct worldcblist_t WorldCallback;

typedef void (*world_callback)(RenderWorld *, GameObject *);

struct animation_key_holder_t
{
    u32 id;
    u32 count;
    AnimationKey **keys;
};
struct animation_key_t
{
    float timeStamp;
    VECTOR key;
};

struct joint_t
{
    char name[MAX_JOINT_NAME];
    u32 id;
    MATRIX offset;
};

struct animation_node_t 
{
    char name[MAX_ANIMATION_NAME];
    u32 childrenCount;
    MATRIX transformation;
    u32 *children;
};

struct animation_data_t
{
    char name[MAX_ANIMATION_NAME];
    float duration;
    float ticksPerSecond;
    u32 numPositionKeys;
    u32 numRotationKeys;
    u32 numScalingKeys;
    AnimationKeyHolder **keyPositions;
    AnimationKeyHolder **keyRotations;
    AnimationKeyHolder **keyScalings;
};

struct animation_mesh_t
{
    u32 jointsCount;
    u32 animationsCount;
    LinkedList *animations;
    Joint **joints;
    VECTOR *finalBones;
    u32 nodeCount;
    AnimationNode *root;
};

struct animator_t
{
    AnimationData *animation;
    float currentTime;
    float deltaTime;
};

enum MeshBuffersType
{
    MESHVERTICES = 0,
    MESHTRIANGLES = 1,
    MESHADJACENCY = 2
};

typedef struct mesh_vectors
{
    u32 vertexCount;
    VECTOR *vertices __attribute__((aligned(128)));
    VECTOR *normals __attribute__((aligned(128)));
    VECTOR *texCoords __attribute__((aligned(128)));
    VectorInt *bones __attribute__((aligned(128)));
    VECTOR *weights __attribute__((aligned(128)));
    VECTOR *colors __attribute__((aligned(128)));
} MeshVectors;

struct mesh_buffers_t
{
    u32 *indices;
    MeshVectors *meshData[2];
    u32 matCount;
    LinkedList *materials;
    AnimationMesh *meshAnimationData;
};

struct material_node_t
{
    u32 start;
    u32 end;
    u64 materialId;
};

enum AnimationStates
{
    ANIM_PAUSE = 0,
    ANIM_RUNNING = 1,
    ANIM_LOOP = 2,
    ANIM_RUNNING_NEXT = 3,
    ANIM_LOOP_NEXT = 4,
    ANIM_STOPPED = 5
};

typedef u32 (*interpolator_callback)(Interpolator *);
typedef void (*morph_target_callback)(MorphTargetBuffer *);

struct morph_target_handle_t
{
    MeshBuffers **morph_targets; // 0 is always base
    morph_target_callback mtCb;
    u16 meshCap;
    u16 meshCount;
    u32 currInterpNode;
    u32 interpCap;
    u32 interpCount;
    Interpolator **interpolators;
    interpolator_callback intCb;
};

struct interpolater_node_t
{
    u32 status;
    u16 begin;
    u16 end;
    float scale;
    float position;
};

struct worldcblist_t
{
    world_callback callback;
    void *args;
    GameObject *obj;
};

enum VU1PipelineLocations
{
    VU1_LOCATION_VIEW_PROJ = 0,
    VU1_LOCATION_GLOBAL_MATRIX = 4,
    VU1_LOCATION_GS_SCALE_VECTOR = 8,
    VU1_LOCATION_RENDERFLAGS = 8,
    VU1_LOCATION_CAM_SCALE_VECTOR = 9,
    VU1_LOCATION_ANIMATION_VECTOR = 9,
    VU1_LOCATION_PRIM_TAG = 10,
    VU1_LOCATION_MATERIAL_COLOR = 11,
    VU1_LOCATION_NEGATIVE_CLIPPING = 14,
    VU1_LOCATION_POSITIVE_CLIPPING = 15,
    VU1_LOCATION_UV_TRANSFORM = 16,
    VU1_LOCATION_LIGHTS_BUFFER = 20,
};

typedef void (*pipeline_callback)(VU1Pipeline *, GameObject *, void *, u32);

struct pipelinecblist_t
{
    u32 id;
    pipeline_callback callback;
    void *args;
    u32 offset;
    /* data */
};

typedef struct plane_t
{
    VECTOR pointInPlane;  // point in plane
    VECTOR planeEquation; // normal and d
} Plane;

struct vu_pipeline_renderpass_t
{
    MeshBuffers *target;
    qword_t programs;
};

struct vu_pipeline_t
{
    char name[MAX_CHAR_PIPELINE_NAME];
    struct vu_pipeline_t *next;
    int callBackSize;
    int numberCBS;
    PipelineCallback **cbs;
    qword_t *q;
    u32 renderPasses;
    u32 qwSize;
    VU1PipelineRenderPass **passes;
};

typedef struct frustum_t
{
    Plane sides[6];
    float nwidth;
    float nheight;
} Frustum;

typedef struct camera_t
{
    float near;
    float far;
    float aspect;
    float angle;
    u32 height;
    u32 width;
    Frustum *frus[2];
    MATRIX view;
    MATRIX proj;
    ObjectBounds *vboContainer;
    MATRIX ltm;
    MATRIX world;
    MATRIX viewProj;
    VECTOR quat;
} Camera;

typedef enum VertexType
{
    V_POS = 1,
    V_COLOR = 2,
    V_NORMAL = 4,
    V_TEXTURE = 8,
    V_SKINNED = 16
} VertexType;

#define RENDER_STATE(draw, cull, alpha_enable, alpha_state,                                                                                        \
                     tex_map, color_enable, z_enable, z_type,                                                                                      \
                     light_enable, bface, clip, envmp,                                                                                             \
                     spec, animtex, morph, bones, alphamap)                                                                                \
    (u32)((draw)&0x00000001) << 0 | (u32)((cull)&0x00000001) << 1 | (u32)((alpha_enable)&0x00000001) << 3 | (u32)((alpha_state)&0x00000003) << 4 | \
        (u32)((tex_map)&0x00000001) << 6 | (u32)((color_enable)&0x00000001) << 7 | (u32)((z_enable)&0x00000001) << 9 |                             \
        (u32)((z_type)&0x00000003) << 10 | (u32)((light_enable)&0x00000001) << 12 | (u32)((bface)&0x00000001) << 13 |                               \
        (u32)((clip)&0x00000001) << 14 | (u32)((envmp)&0x00000001) << 15 | (u32)((spec)&0x00000001) << 16 | (u32)((animtex)&0x00000001) << 17 |    \
        (u32)((morph)&0x00000001) << 18 | (u32)((bones)&0x00000001) << 8 | (u32)((alphamap)&0x00000001) << 19

typedef union obj_render_properties
{
    struct
    {
        unsigned int DRAWING_OPTION : 1;     // 1
        unsigned int CULLING_OPTION : 1;     // 2
        unsigned int CULLING_VISIBLE : 1;    // 3
        unsigned int ALPHA_ENABLE : 1;       // 4
        unsigned int ALPHA_STATE : 2;        // 5-6
        unsigned int TEXTURE_MAPPING : 1;    // 7
        unsigned int COLOR_ENABLE : 1;       // 8
        unsigned int SKELETAL_ANIMATION : 1; // 9
        unsigned int Z_ENABLE : 1;           // 10
        unsigned int Z_TYPE : 2;             // 11-12
        unsigned int LIGHTING_ENABLE : 1;    // 13
        unsigned int BACKFACE_CULLING : 1;   // 14
        unsigned int CLIPPING : 1;           // 15
        unsigned int ENVIRONMENTMAP : 1;     // 16
        unsigned int SPECULAR : 1;           // 17
        unsigned int ANIMATION_TEXUTRE : 1;  // 18
        unsigned int MORPH_TARGET : 1;       // 19
        
        unsigned int ALPHA_MAPPING : 1;      // 20
        unsigned int pad : 12;               // 21-32
    };

    unsigned int props;

} ObjectProperties;

typedef struct obj_gs_state_t
{
    int gs_reg_count;
    int gs_reg_mask;
    prim_t prim;
} ObjGSState;

typedef struct render_state_t
{
    Color color;
    ObjectProperties properties;
    ObjGSState gsstate;
} ObjectRenderState;

enum Lighting
{
    PS_AMBIENT_LIGHT = 0,
    PS_DIRECTIONAL_LIGHT = 1,
    PS_POINT_LIGHT = 2,
    PS_SPOT_LIGHT = 3,
};

typedef struct lights_t
{
    MATRIX ltm;
    VECTOR color;
    float theta;
    float radius;
    u32 type;
} LightStruct;

struct render_world_t
{
    u32 objectCount;
    u32 lightCount;

    LinkedList *objList;
    LinkedList *lights;

    Camera *cam;
};

struct gameobject_t
{
    MATRIX ltm;
    MATRIX world;
    MeshBuffers vertexBuffer;
    void (*update_object)(struct gameobject_t *);
    void *objData;
    ObjectBounds *vboContainer;
    VU1Pipeline *pipelines;
    VU1Pipeline *activePipeline;
    ObjectRenderState renderState;
    MorphTargetBuffer *interpolator;
    Animator *objAnimator;
};

typedef struct RenderTarget
{
    framebuffer_t *render;
    zbuffer_t *z;
    u32 memInVRAM;
} RenderTarget;

typedef struct avltree_t
{
    u64 node; 
    u32 height;
    struct avltree_t *left, *right;
    void *data;
} AVLTree;

typedef u64 (*HashFunction) (const char *, int);
typedef struct hashmap_t
{
    int cap;
    HashFunction func;
    AVLTree* trees;
} HashMap;

typedef struct hashentry_t
{
    const char *key;
    int sizeKey;
    void *data;
} HashEntry;

typedef struct
{
    u32 count;        // global tex count
    HashMap *textureMap;
} TextureManager;


struct timer_struct_t;
typedef struct timer_struct_t TimerStruct;

struct timer_struct_t
{
    u64 ctr;
    s32 id;
};

typedef struct vram_manager_t
{
    int vramSize;
    int systemVRAMUsed;
    int userVRAMUsed;
    int currentTextureBasePtr;
    LinkedList *renderTargets;
} VRAMManager;


typedef struct manager_info_t
{
    u32 height;
    u32 width;
    u32 psm;
    bool zenable;
    bool doublebuffered;
    u32 zsm;
    u32 drawBufferSize;
    bool fsaa;
    u32 vu1programsize;
} ManagerInfo;

typedef struct draw_buffers_t {
    qword_t *vifupload[2];
    qword_t *gifupload[2];
    qword_t *currentvif; //where the write head is VIF
    qword_t *currentgif; //where the write head is GIF
    qword_t *readvif;
    qword_t *readgif;
    u32 size;
    u32 context;
} DrawBuffers;

typedef struct dma_buffers_t 
{
    qword_t *tosprtape;
    qword_t *tospr;
} DMABuffers;

typedef struct
{
    RenderTarget *targetBack;
    RenderTarget *targetDisplay;
    TextureManager *texManager;
    VU1Manager *vu1Manager;
    DrawBuffers *drawBuffers;
    DMABuffers *dmaBuffers;
    VRAMManager *vramManager;
    bool vu1DoneProcessing;
    u16 gs_context;
    bool enableDoubleBuffer;
    bool fsaaEnable;
    Color bgkc;
    u32 ScreenWidth, ScreenHeight;
    u32 ScreenWHalf, ScreenHHalf;
    float lastTime, currentTime;
    u32 FPS;
    TimerStruct *timer;
} GameManager;

typedef struct font_t
{
    Texture *fontTex;
    char *fontWidths;
    Color color;
    prim_t prim;
    u16 picHeight;
    u16 picWidth;
    u8 cellWidth;
    u8 cellHeight;
    u16 startingChar;
    u32 widthSize;
} Font;

typedef struct wingedtriangle_t
{
    u32 v1, v2, v3;
    s32 t1, t2, t3;
    VECTOR plane;
} WingedTriangle;

typedef WingedTriangle* FaceVertexTable;

#define EPSILON 0.0001

// Global Variables

extern RenderWorld *g_DrawWorld; // active render world. PS_RenderWorld.c
extern GameManager g_Manager;    // PS2 render manager PS_Manager.c
extern Camera *g_DrawCamera;     // current drawing camra PS_Camera.c


// vu1 data address VU1Manager.c
extern volatile u32 *vu1_data_address;

extern volatile u32 *vu0_data_address;

extern volatile u32 *vif1_top;

extern volatile u32 *vif1_tops;

#endif
