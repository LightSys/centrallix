package org.lightsys.centrallix.objectsystem;

public class CObjectInstance {

    public static CObjectInstance fromJava(ObjectInstance obj){
        if(obj instanceof CObjectInstance){
            return (CObjectInstance) obj;
        }
        // TODO provide some sort of caching mechanism so that we only create
        //      one instance of CObjectInstance per ObjectInstance instance
        return new CObjectInstance(obj);
    }
}
