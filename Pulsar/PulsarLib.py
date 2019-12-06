def generate(env, **kw):
    if not kw.get('depsOnly',0):
        env.Tool('addLibrary', library = ['Pulsar'])
        if env['PLATFORM'] == 'win32' and env.get('CONTAINERNAME','')=='GlastRelease':
            env.Tool('findPkgPath', package = 'Pulsar') 
            env.Tool('findPkgPath', package = 'SpectObj') 
            env.Tool('findPkgPath', package = 'flux') 
            env.Tool('findPkgPath', package = 'astro') 
            env.Tool('findPkgPath', package = 'facilities') 
    env.Tool('fluxLib')
    env.Tool('SpectObjLib')
    env.Tool('astroLib')
    env.Tool('addLibrary', library = env['rootLibs'])
    env.Tool('addLibrary', library = env['rootGuiLibs'])
    if kw.get('incsOnly', 0) == 1: 
        env.Tool('findPkgPath', package = 'SpectObj') 
        env.Tool('findPkgPath', package = 'flux') 
        env.Tool('findPkgPath', package = 'astro') 
        env.Tool('findPkgPath', package = 'facilities') 

def exists(env):
    return 1
