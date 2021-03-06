
Scene:
    name: Main
    
    Resources:
        FileSystem: ['%(RAM_SVN_DIR)s/tools/simulator/data/media/materials/scripts',
                     '%(RAM_SVN_DIR)s/tools/simulator/data/media/materials/textures',
                     '%(RAM_SVN_DIR)s/tools/simulator/data/media/models',
                     '%(RAM_SVN_DIR)s/tools/simulator/data/media/models/primitives']
                         
    #SkyBox:
    #    material_name: 'Examples/CloudyNoonSkyBox'
        
    Lights:
        light1:
            type: LT_POINT
            position: [-20, -20, 20]
            colour: [0.5, 0.5, 0.5]
    ambient_light_colour: [0.5, 0.5, 0.5]
    
    Cameras:
        Main:
            position: [0, -22.82, 0]
            #position: [22.82, 0, 10]
            #offset: [0, 0, 2]
            offset: [0, 0, 70]
    
    Robots:
        Tortuga: 
            path: '%(RAM_SVN_DIR)s/tools/simulator/data/robots/tortuga3.rml'
            position: [-23, -15, 0]
            orientation: [0, 0, 1, -55]
            #position: [-25, -1, 0]
            #orientation: [0, 0, 1, 60]
            #position: [-22, -19, -3.05]
    
    Objects:
        course:
            type: [sim.vision.ICourse, sim.vision.Course]

            startType: [sim.vision.IPipe, sim.vision.Pipe]
            start: pipe1
            startPos: [-16, -24]
            startDepth: -4.2672
            startHeading: 40

            pipe1:        
                greenBuoy:
                    type: [sim.vision.IBuoy, sim.vision.Buoy]
                    distance: 5.5
                    depth: -2.438
                    offset: -0.5
                    Graphical:
                        material: Simple/Green

                yellowBuoy:
                    type: [sim.vision.IBuoy, sim.vision.Buoy]
                    distance: 6
                    depth: -1.981
                    offset: 0.5
                    Graphical:
                        material: Simple/Yellow

                redBuoy:
                    type: [sim.vision.IBuoy, sim.vision.Buoy]
                    distance: 5.5
                    depth: -1.524
                    Graphical:
                        material: Simple/Red

            greenBuoy:
                pipe2:
                    type: [sim.vision.IPipe, sim.vision.Pipe]
                    distance: 3
                    depth: -4.2672
                    heading: -15
                    offset: -0.5

            pipe2:
                hedge1:
                    type: [sim.vision.IHedge, sim.vision.Hedge]
                    distance: 6
                    depth: -2.469
                    heading: -5

            hedge1:
                rightpipe:
                    type: [sim.vision.IPipe, sim.vision.Pipe]
                    distance: 2
                    heading: 10
                    offset: -0.5
                    depth: -4.2672
                leftpipe:
                    type: [sim.vision.IPipe, sim.vision.Pipe]
                    distance: 1.8
                    heading: -45
                    offset: 0.4
                    depth: -4.2672

            leftpipe:
                bin:
                    type: [ram.sim.object.IObject, sim.vision.BlackJackTable]
                    distance: 6
                    heading: 0
                    depth: -3.9
                    symbols: ['axe', 'hammer', 'machete', 'clippers']

            bin:
                targetPipe:
                    type: [sim.vision.IPipe, sim.vision.Pipe]
                    distance: 0.3
                    heading: 100
                    offset: -3
                    depth: -4.2672
                binSonarPipe:
                    type: [sim.vision.IPipe, sim.vision.Pipe]
                    distance: 1.5
                    heading: 10
                    depth: -4.2672

            windows:
                binPipe:
                    type: [sim.vision.IPipe, sim.vision.Pipe]
                    distance: -1
                    heading: -135
                    offset: 1
                    depth: -4.2672
                targetSonarPipe:
                    type: [sim.vision.IPipe, sim.vision.Pipe]
                    distance: 1.5
                    heading: -60
                    offset: 0.5
                    depth: -4.2672

            rightpipe:
                windows:
                    type: [ram.sim.object.IObject, sim.vision.TargetArray]
                    distance: 9
                    heading: 5
                    depth: -2.591
                    colors: ['yellow', 'blue', 'green', 'red']

            targetSonarPipe:
                hedge3:
                    type: [sim.vision.IHedge, sim.vision.Hedge]
                    distance: 15.5
                    depth: -2.469
                    heading: -30
           
            hedge3:
                fakeSafe:
                    type: [sim.vision.ISafe, sim.vision.Safe]
                    distance: 7
                    depth: -3.048

            binSonarPipe:
                hedge2:
                    type: [sim.vision.IHedge, sim.vision.Hedge]
                    distance: 14
                    depth: -2.469
                    heading: -15

            hedge2:
                sonar:
                    type: [sim.sonar.IPinger, sim.sonar.Pinger]
                    distance: 7.5
                    depth: -4.572

                safe:
                    type: [sim.vision.ISafe, sim.vision.Safe]
                    distance: 7.5
                    depth: -3.048

        course2:
            type: [sim.vision.ICourse, sim.vision.Course]

            startType: [sim.vision.IPipe, sim.vision.Pipe]
            start: pipe3
            startPos: [-22, 6]
            startDepth: -3.05
            startHeading: -60

            pipe3:        
                buoy2:
                    type: [sim.vision.IBuoy, sim.vision.Buoy]
                    distance: 6
                    depth: -1.425

            buoy2:
                pipe4:
                    type: [sim.vision.IPipe, sim.vision.Pipe]
                    distance: 2
                    depth: -4.2672
                    heading: 10

            pipe4:
                barbedwire2:
                    type: [sim.vision.IBarbedWire, sim.vision.BarbedWire]
                    distance: 7
                    depth: -0.953
                    heading: -5

            barbedwire2:
                rightpipe2:
                    type: [sim.vision.IPipe, sim.vision.Pipe]
                    distance: 2
                    heading: 70
                    offset: -0.7
                    depth: -4.2672
                leftpipe2:
                    type: [sim.vision.IPipe, sim.vision.Pipe]
                    distance: 2
                    heading: -5
                    offset: 0.5
                    depth: -4.2672

            leftpipe2:
                bin2:
                    type: [ram.sim.object.IObject, sim.vision.BlackJackTable]
                    distance: 6
                    heading: 20
                    depth: -3.9
                    symbols: ['ship', 'tank', 'aircraft', 'factory']

            rightpipe2:
                target2:
                    type: [ram.sim.object.IObject, sim.vision.Target]
                    distance: 8
                    heading: -30
                    depth: -1.67

        tank_walls:
            type: [ram.sim.scene.ISceneObject, ram.sim.scene.SceneObject]
            position: [0, 0, 0]
            orientation: [0.97028687514552081, 0.17108808730113592, 0.17108808730113592, 91.727979443648806]
            
            Graphical:
                mesh: 'TankWallsMesh.mesh'
                scale: [.3048, .3048, .3048]  # convert feet to meters
                material: 'CompElement/TankWalls'
            
            Physical:
                mass: 0
                Shape:
                    #type: box
                    #size: [0.6858, 0.2032, 0.2032]
                    type: mesh
                    mesh_name: 'TankWallsMesh.mesh'
        
#        water:
#            type: [ram.sim.graphics.IVisual, ram.sim.graphics.Visual]
#            position: [0, 0, -1]
#            orientation: [1, 0, 0, 90]
#            
#            Graphical:
#                mesh: 'WaterSurfaceMesh.mesh'
#                scale: [.3048, .3048, .3048]  # convert feet to meters
#                material: 'Simple/Translucent'
