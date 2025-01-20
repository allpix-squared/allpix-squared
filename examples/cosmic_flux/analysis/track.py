import numpy as np

class Track(object):

    def __init__(self, state : np.array, direction : np.array, chi2, ndof, chi2ndof, timestamp, middleOfDetector = np.array([0,0, -1550])):
        self.state = state
        self.direction = direction
        self.chi2 = chi2
        self.ndof = ndof
        self.chi2ndof = chi2ndof
        self.timestamp = timestamp
        self.middleOfDetector = middleOfDetector


        self.closestApproach = self._distanceToPoint(self.middleOfDetector)
        self.zenithAngle     = self._zenithAngle(degrees = True)
        self.azimuthalAngle  = self._azimuthalAngle(degrees = True)

    def _distanceToPoint(self, point : np.array):
        upper = np.cross(self.state - point, self.direction)
        upper = np.linalg.norm(upper)
        lower = np.linalg.norm(self.direction)
        return upper / lower

    def _distanceToZAxis(self):
        zAxisState = np.array([0,0,0])
        zAxisDirection = np.array([0,0,1])

        crossProduct = np.cross(zAxisDirection, self.direction)
        difference = zAxisState - self.state
        dotProduct = np.dot(difference, crossProduct)

        return np.linalg.norm(dotProduct) / np.linalg.norm(crossProduct)

    def _zenithAngle(self, degrees = True):
        normalZ = np.array([0,0,1])
        inner = np.inner(self.direction, normalZ)
        norms = np.linalg.norm(self.direction) * np.linalg.norm(normalZ)

        cos = inner / norms
        rad = np.arccos(np.clip(cos, -1.0, 1.0))
        if degrees:
            return np.rad2deg(rad)
        else:
            return rad

    def _azimuthalAngle(self, degrees = True):
        normalX = np.array([1,0,0])
        projectedDirection = - np.array([self.direction[0], self.direction[1], 0])
        inner = np.inner(projectedDirection, normalX)
        norms = np.linalg.norm(projectedDirection) * np.linalg.norm(normalX)

        cos = inner / norms
        rad = np.arccos(np.clip(cos, -1.0, 1.0))
        if degrees:
            return np.rad2deg(rad)
        else:
            return rad

    def COMIntersection(self):
        intersection = self.state + self.middleOfDetector[2] * self.direction
        return intersection[0], intersection[1]
