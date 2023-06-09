#include "ps_renderworld.h"
#include "ps_manager.h"
#include "ps_vu1pipeline.h"
#include "ps_lights.h"
#include "ps_vumanager.h"
#include "ps_camera.h"
#include "ps_gameobject.h"
#include "ps_morphtarget.h"
#include <ps_misc.h>
#include "ps_log.h"
#include <stdlib.h>

RenderWorld *g_DrawWorld = NULL;

RenderWorld *CreateRenderWorld()
{
    RenderWorld *ret = (RenderWorld *)malloc(sizeof(RenderWorld));
    ret->lightCount = ret->objectCount = 0;
    ret->objList = ret->lights = NULL;
    return ret;
}

void DestoryRenderWorld(RenderWorld *world)
{
    world->lights = CleanWorldLightsList(world);
    world->objList = CleanWorldObjList(world);
    free(world);
}

LinkedList *CleanWorldLightsList(RenderWorld *world)
{
    LinkedList *iter = world->lights;

    while (iter->next != NULL)
    {
        LinkedList *clean = iter;
        iter = iter->next;

        if (clean->data)
        {
            free(clean->data);
        }

        CleanLinkedListNode(clean);
    }

    if (iter->data)
    {
        free(iter->data);
    }

    CleanLinkedListNode(iter);

    world->lights = NULL;

    return world->lights;
}

LinkedList *CleanWorldObjList(RenderWorld *world)
{
    LinkedList *iter = world->objList;

    while (iter->next != NULL)
    {
        LinkedList *clean = iter;
        iter = iter->next;
        CleanGameObject(clean->data);
        CleanLinkedListNode(clean);
    }

    CleanGameObject(iter->data);
    CleanLinkedListNode(iter);

    world->objList = NULL;

    return world->objList;
}

void AddLightToRenderWorld(RenderWorld *world, LightStruct *light)
{
    LinkedList *node = CreateLinkedListItem((void *)light);
    world->lights = AddToLinkedList(world->lights, node);
    world->lightCount += 1;
}

void AddObjectToRenderWorld(RenderWorld *world, GameObject *obj)
{
    LinkedList *node = CreateLinkedListItem((void *)obj);
    world->objList = AddToLinkedList(world->objList, node);
    world->objectCount += 1;
}

RenderWorld *RemoveLightFromRenderWorld(RenderWorld *world, LightStruct *light)
{
    LinkedList *iter = world->lights;
    while ((LightStruct *)iter->data != light)
    {
        iter = iter->next;
    }
    world->lights = RemoveNodeFromList(world->lights, iter);
    world->lightCount -= 1;
    return world;
}

RenderWorld *RemoveObjectFromRenderWorld(RenderWorld *world, GameObject *obj)
{
    LinkedList *iter = world->objList;
    GameObject *comp = (GameObject *)iter->data;
    while (comp != obj && iter != NULL)
    {
        iter = iter->next;
        comp = (GameObject *)iter->data;
    }

    if (iter == NULL)
        return world;

    INFOLOG("found");
    world->objList = RemoveNodeFromList(world->objList, iter);
    world->objectCount -= 1;
    return world;
}

void DrawWorld(RenderWorld *world)
{
    LinkedList *node = world->objList;

    int i = 0;

    g_DrawWorld = world;

    while (node != NULL)
    {

        GameObject *obj = (GameObject *)node->data;
        if (obj->interpolator != NULL)
        {
            ExecuteMorphTargetCBFuncs(obj->interpolator);
        }

        if (obj->update_object != NULL)
        {
            obj->update_object(obj);
        }

        int draw = (obj->renderState.state.render_state.CULLING_OPTION == 0 ? 1 : TestObjectInCameraFrustum(world->cam, obj));

        if (draw != 0)
        {
            RenderPipeline(obj, obj->activePipeline);
        }

        ClearDirtyLTM(obj->ltm);

        node = node->next;
    }

    g_DrawWorld = NULL;
}

RenderWorld *AddWorldCallback(RenderWorld *world)
{
    return world;
}